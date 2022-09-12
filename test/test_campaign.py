import json
from pathlib import Path
import shlex
from unittest.mock import patch, mock_open, call, MagicMock

import pytest

from binrec import campaign
from binrec.env import BINREC_PROJECTS

from helpers.mock_path import MockPath

BATCH_INVOCATIONS = [
    [{
        "arg_type": "concrete",
        "value": "a"
    },
    {
        "arg_type": "concrete",
        "value": "a"
    }],
    [{
        "arg_type": "concrete",
        "value": "a"
    },
    {
        "arg_type": "concrete",
        "value": "b"
    }]
]


JSON_BATCH = {
    "traces": [{"args": args} for args in BATCH_INVOCATIONS]
}
JSON_INPUT_FILES = {
    "traces": [{
        "input_files": ["hello.txt"]
    }]
}

FULL_BOOTSTRAP_FILE = """#!/bin/bash
stuff
    S2E_SYM_ARGS="" LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null
other stuff
execute "${TARGET_PATH}" zz
more stuff
"""

FULL_BOOTSTRAP_FILE_PATCHED = f"""#!/bin/bash
stuff
{campaign.BOOTSTRAP_PATCH_EXECUTE_SAMPLE}other stuff
{campaign.BOOTSTRAP_PATCH_LOAD_TRACE_CONFIG}{campaign.BOOTSTRAP_PATCH_CALL_EXECUTE}more stuff

{campaign.BOOTSTRAP_PATCH_MARKER}
"""

BOOTSTRAP_MISSING_CALL = """#!/bin/bash
stuff
    S2E_SYM_ARGS="" LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null
other stuff
more stuff
"""

BOOTSTRAP_MISSING_EXECUTE = """#!/bin/bash
stuff
other stuff
execute "${TARGET_PATH}" zz
more stuff
"""

class TestTraceParams:

    def test_args(self):
        c = campaign.TraceParams(args=[
            campaign.TraceArg(campaign.TraceArgType.symbolic, "first"),
            campaign.TraceArg(campaign.TraceArgType.concrete, "second"),
            campaign.TraceArg(campaign.TraceArgType.symbolic, "")
        ])
        assert c.symbolic_indexes == [1, 3]
        assert c.command_line_args == ["first", "second", campaign.DEFAULT_SYMBOLIC_ARG_VALUE]

    @patch.object(campaign, "open", new_callable=mock_open)
    @patch.object(campaign, "print")
    @patch.object(campaign.TraceParams, "_write_variables")
    @patch.object(campaign.TraceParams, "_write_get_input_files_function")
    @patch.object(campaign.TraceParams, "_write_setup_function")
    @patch.object(campaign.TraceParams, "_write_teardown_function")
    def test_write_config_script(self, mock_teardown, mock_setup, mock_input_files, mock_vars, mock_print, mock_file):
        filename = BINREC_PROJECTS / "asdf" / "binrec_trace_config.sh"
        params = campaign.TraceParams()
        params.write_config_script("asdf")

        mock_file.assert_called_once_with(filename, "w")
        handle = mock_file()
        mock_print.assert_called_once_with("#!/bin/bash", file=handle)
        mock_vars.assert_called_once_with(handle)
        mock_input_files.assert_called_once_with(handle)
        mock_setup.assert_called_once_with(handle)
        mock_teardown.assert_called_once_with(handle)

    @patch.object(campaign, "shlex")
    @patch.object(campaign, "print")
    def test_write_variables(self, mock_print, mock_shlex):
        handle = MagicMock()
        mock_shlex.quote.side_effect = ["sym", "con_1", "con_2"]
        params = campaign.TraceParams(args=[
            campaign.TraceArg(campaign.TraceArgType.symbolic, "one"),
            campaign.TraceArg(campaign.TraceArgType.symbolic, "two"),
        ])
        params._write_variables(handle)

        assert mock_shlex.quote.call_args_list == [call("1 2"), call("one"), call("two")]
        assert mock_print.call_args_list == [
            call("export S2E_SYM_ARGS=sym", file=handle),
            call("export TRACE_ARGS=(con_1 con_2)", file=handle),
            call(file=handle),
        ]

    @patch.object(campaign, "print")
    def test_write_get_input_files_functions(self, mock_print):
        handle = MagicMock()
        mock_file_input = MagicMock()
        params = campaign.TraceParams(input_files=[mock_file_input])
        params._write_get_input_files_function(handle)

        assert mock_print.call_args_list == [
            call("function get_trace_input_files() {", file=handle),
            call("  mkdir ./input_files", file=handle),
            call("  cd ./input_files", file=handle),
            call(mock_file_input.get_file_script.return_value, file=handle),
            call("  cd ..", file=handle),
            call("}", file=handle),
            call(file=handle)
        ]
        mock_file_input.get_file_script.assert_called_once_with(s2e_get_var="../${S2EGET}", indent="  ")

    @patch.object(campaign, "print")
    def test_write_setup_function(self, mock_print):
        handle = MagicMock()
        params = campaign.TraceParams(setup=['asdf'])
        params._write_setup_function(handle)

        assert mock_print.call_args_list == [
            call("function setup_trace() {", file=handle),
            call("  asdf", file=handle),
            call("  return", file=handle),
            call("}", file=handle),
            call(file=handle)
        ]

    @patch.object(campaign, "print")
    def test_write_teardown_function(self, mock_print):
        handle = MagicMock()
        params = campaign.TraceParams(teardown=['asdf'])
        params._write_teardown_function(handle)

        assert mock_print.call_args_list == [
            call("function teardown_trace() {", file=handle),
            call("  asdf", file=handle),
            call("  return", file=handle),
            call("}", file=handle),
            call(file=handle)
        ]

    def test_load_dict(self):
        assert campaign.TraceParams.load_dict({
            "args": ["asdf"],
            "stdin": True,
            "match_stdout": False,
            "match_stderr": False,
            "extra": "ignore me"
        }) == campaign.TraceParams(
            args=[campaign.TraceArg(campaign.TraceArgType.concrete, "asdf")],
            stdin=True,
            match_stdout=False,
            match_stderr=False
        )

    def test_load_dict_default(self):
        assert campaign.TraceParams.load_dict({}) == campaign.TraceParams(
            args=[],
            stdin=False,
            match_stdout=True,
            match_stderr=True
        )

    @patch.object(campaign.TraceInputFile, "load_dict")
    def test_load_dict_file_inputs(self, mock_file_load_dict):
        assert campaign.TraceParams.load_dict({
            "input_files": ["thing.py", {"source": "other.py"}]
        }).input_files == [
            campaign.TraceInputFile(Path("thing.py")),
            mock_file_load_dict.return_value
        ]
        mock_file_load_dict.assert_called_once_with({"source": "other.py"})

    def test_load_dict_invalid_input_file(self):
        with pytest.raises(ValueError):
            campaign.TraceParams.load_dict({
                "input_files": [100]
            })

    def test_load_dict_invalid_input_file_array(self):
        with pytest.raises(ValueError):
            campaign.TraceParams.load_dict({
                "input_files": "asdf"
            })

    @patch.object(campaign, "input_files_dir")
    def test_setup_input_file_directory(self, mock_input_files_dir):
        dirname = mock_input_files_dir.return_value = MockPath("/root", is_dir=True)
        sources = [
            MockPath("setup.sh", exists=True),
            MockPath("thing.cfg", exists=True)
        ]
        campaign.TraceParams(input_files=[MagicMock(source=src) for src in sources]).setup_input_file_directory("asdf")
        (dirname / "setup.sh").symlink_to.assert_called_once_with(sources[0])
        (dirname / "thing.cfg").symlink_to.assert_called_once_with(sources[1])
        dirname.mkdir.assert_not_called()

    @patch.object(campaign, "input_files_dir")
    def test_setup_input_file_directory_not_exists(self, mock_input_files_dir):
        dirname = mock_input_files_dir.return_value = MockPath("/root", exists=False)
        campaign.TraceParams().setup_input_file_directory("asdf")
        dirname.mkdir.assert_called_once()

    @patch.object(campaign, "input_files_dir")
    def test_setup_input_file_directory_cleanup(self, mock_input_files_dir):
        dirname = mock_input_files_dir.return_value = MockPath("/root", is_dir=True)
        symlink = MockPath("thing", exists=True, is_symlink=MagicMock(return_value=True))
        not_symlink = MockPath("thing2", exists=True, is_symlink=MagicMock(return_value=False))
        dirname.build_tree(symlink, not_symlink)
        campaign.TraceParams().setup_input_file_directory("asdf", cleanup=True)
        symlink.unlink.assert_called_once()
        not_symlink.unlink.assert_not_called()

    @patch.object(campaign, "input_files_dir")
    def test_setup_input_file_directory_no_cleanup(self, mock_input_files_dir):
        dirname = mock_input_files_dir.return_value = MockPath("/root", is_dir=True)
        symlink = MockPath("thing", exists=True, is_symlink=MagicMock(return_value=True))
        not_symlink = MockPath("thing2", exists=True, is_symlink=MagicMock(return_value=False))
        dirname.build_tree(symlink, not_symlink)
        campaign.TraceParams().setup_input_file_directory("asdf", cleanup=False)
        symlink.unlink.assert_not_called()
        not_symlink.unlink.assert_not_called()

    def test_create_trace_args(self):
        assert campaign.TraceParams.create_trace_args(["first", "second"], [2]) == [
            campaign.TraceArg(campaign.TraceArgType.concrete, "first"),
            campaign.TraceArg(campaign.TraceArgType.symbolic, "second"),
        ]

    def test_create_trace_args_error_gt_len(self):
        with pytest.raises(IndexError):
            assert campaign.TraceParams.create_trace_args(["first", "second"], [3])

    def test_create_trace_args_error_zero(self):
        with pytest.raises(IndexError):
            assert campaign.TraceParams.create_trace_args(["first", "second"], [0])


class TestCampaign:

    def test_project(self):
        assert campaign.Campaign(binary=Path("/eq"), traces=[]).project == "eq"

    @patch.object(campaign, "open", new_callable=mock_open, read_data=json.dumps(JSON_BATCH))
    def test_load_json(self, mock_file):
        binary = Path("/eq")
        filename = Path("thing.json")
        project_name = "asdf"
        assert campaign.Campaign.load_json(binary, filename, project_name) == campaign.Campaign(
            binary,
            traces=[
                campaign.TraceParams(args=[
                    campaign.TraceArg(campaign.TraceArgType.concrete, "a"),
                    campaign.TraceArg(campaign.TraceArgType.concrete, "a")]),
                campaign.TraceParams(args=[
                    campaign.TraceArg(campaign.TraceArgType.concrete, "a"),
                    campaign.TraceArg(campaign.TraceArgType.concrete, "b")]),
            ],
            project=project_name
        )
        mock_file.assert_called_once_with(filename, "r")
        mock_file().read.assert_called_once_with()

    @patch.object(campaign, "open", new_callable=mock_open, read_data=json.dumps(JSON_INPUT_FILES))
    @patch.object(campaign, "TraceInputFile")
    def test_load_json_resolve_input_files(self, mock_input_file_cls, mock_file):
        binary = Path("/eq")
        filename = Path("/path/to/thing.json")
        project_name = "asdf"
        assert campaign.Campaign.load_json(binary, filename, project_name) == campaign.Campaign(
            binary,
            traces=[
                campaign.TraceParams(input_files=[mock_input_file_cls.return_value])
            ],
            project=project_name
        )
        mock_file.assert_called_once_with(filename, "r")
        mock_file().read.assert_called_once_with()
        mock_input_file_cls.return_value.check_source.assert_called_once_with(filename.parent)


class TestPatchProject:

    @patch.object(campaign, "open", new_callable=mock_open, read_data=FULL_BOOTSTRAP_FILE)
    def test_full_case(self, mock_file):
        assert campaign.patch_s2e_project("asdf") is True
        handle = mock_file()
        write_content = "".join(call_args.args[0] for call_args in handle.write.mock_calls)
        print(write_content)
        assert write_content == FULL_BOOTSTRAP_FILE_PATCHED

    @patch.object(campaign, "open", new_callable=mock_open, read_data=f"{FULL_BOOTSTRAP_FILE}\n{campaign.BOOTSTRAP_PATCH_MARKER}")
    def test_full_case_exists(self, mock_file):
        with pytest.raises(ValueError):
            campaign.patch_s2e_project("asdf", existing_patch_ok=False)

    @patch.object(campaign, "open", new_callable=mock_open, read_data=f"{FULL_BOOTSTRAP_FILE}\n{campaign.BOOTSTRAP_PATCH_MARKER}")
    def test_full_case_exists_ok(self, mock_file):
        assert campaign.patch_s2e_project("asdf", existing_patch_ok=True) is False
        mock_file().write.assert_not_called()

    @patch.object(campaign, "open", new_callable=mock_open, read_data=BOOTSTRAP_MISSING_CALL)
    def test_missing_call(self, mock_file):
        with pytest.raises(ValueError):
            campaign.patch_s2e_project("asdf")

    @patch.object(campaign, "open", new_callable=mock_open, read_data=BOOTSTRAP_MISSING_EXECUTE)
    def test_missing_execute(self, mock_file):
        with pytest.raises(ValueError):
            campaign.patch_s2e_project("asdf")

    @patch.object(campaign, "open", new_callable=mock_open)
    def test_save_str(self, mock_file):
        binary = Path("/eq")
        c = campaign.Campaign(binary=binary)
        c.save("./camp.j")
        mock_file.assert_called_once_with("./camp.j", "w")
        mock_file().write.assert_called_once_with(json.dumps({
            "traces": [],
            "setup": [],
            "teardown": []
        }, indent=2))

    @patch.object(campaign, "open", new_callable=mock_open)
    def test_save_path(self, mock_file):
        binary = Path("/eq")
        c = campaign.Campaign(binary=binary)
        dest = Path("/camp.j")
        c.save(dest)
        mock_file.assert_called_once_with(dest, "w")
        mock_file().write.assert_called_once_with(json.dumps({
            "traces": [],
            "setup": [],
            "teardown": []
        }, indent=2))

    def test_save_file(self):
        binary = Path("/eq")
        c = campaign.Campaign(binary=binary)
        dest = MagicMock()
        c.save(dest)
        dest.write.assert_called_once_with(json.dumps({
            "traces": [],
            "setup": [],
            "teardown": []
        }, indent=2))

    @patch.object(campaign, "open", new_callable=mock_open)
    @patch.object(campaign, "campaign_filename")
    def test_save_default(self, mock_filename, mock_file):
        binary = Path("/eq")
        mock_filename.return_value = "/campaign.json"
        c = campaign.Campaign(binary=binary, project="asdf")
        c.save()
        mock_filename.assert_called_once_with("asdf")
        mock_file.assert_called_once_with(mock_filename.return_value, "w")
        mock_file().write.assert_called_once_with(json.dumps({
            "traces": [],
            "setup": [],
            "teardown": []
        }, indent=2))

    def test_remove_trace_str(self):
        c = campaign.Campaign(MagicMock(), traces=[
            campaign.TraceParams(),
            campaign.TraceParams(name="asdf"),
            campaign.TraceParams(name="qwer"),
        ])
        c.remove_trace("asdf")
        assert c.traces == [
            campaign.TraceParams(),
            campaign.TraceParams(name="qwer"),
        ]

    def test_remove_trace_str_error(self):
        c = campaign.Campaign(MagicMock(), traces=[
            campaign.TraceParams(name="asdf"),
        ])

        with pytest.raises(KeyError):
            c.remove_trace("qwer")

    def test_remove_trace_int(self):
        c = campaign.Campaign(MagicMock(), traces=[
            campaign.TraceParams(),
            campaign.TraceParams(name="asdf"),
            campaign.TraceParams(name="qwer"),
        ])
        c.remove_trace(1)
        assert c.traces == [
            campaign.TraceParams(),
            campaign.TraceParams(name="qwer"),
        ]

    def test_remove_trace_int_error(self):
        c = campaign.Campaign(MagicMock(), traces=[
            campaign.TraceParams(name="asdf"),
        ])

        with pytest.raises(IndexError):
            c.remove_trace(1)

    def test_get_trace_str(self):
        c = campaign.Campaign(MagicMock(), traces=[
            campaign.TraceParams(),
            campaign.TraceParams(name="asdf"),
            campaign.TraceParams(name="qwer"),
        ])
        assert c.get_trace("asdf") is c.traces[1]

    def test_get_trace_str_error(self):
        c = campaign.Campaign(MagicMock(), traces=[
            campaign.TraceParams(name="asdf"),
        ])

        with pytest.raises(KeyError):
            c.get_trace("qwer")

    def test_get_trace_int(self):
        c = campaign.Campaign(MagicMock(), traces=[
            campaign.TraceParams(),
            campaign.TraceParams(name="asdf"),
            campaign.TraceParams(name="qwer"),
        ])
        assert c.get_trace(1) is c.traces[1]

    def test_get_trace_int_error(self):
        c = campaign.Campaign(MagicMock(), traces=[
            campaign.TraceParams(name="asdf"),
        ])

        with pytest.raises(IndexError):
            c.get_trace(1)

    @patch.object(campaign.Campaign, "load_json")
    @patch.object(campaign, "project_binary_filename")
    @patch.object(campaign, "campaign_filename")
    def test_load_project(self, mock_campaign, mock_proj, mock_load):
        project_name = "asdf"
        campaign.Campaign.load_project(project_name, x=1)
        mock_campaign.assert_called_once_with(project_name)
        mock_proj.assert_called_once_with(project_name)
        mock_load.assert_called_once_with(mock_proj.return_value, mock_campaign.return_value, project=project_name, x=1)


class TestTraceInputFile:

    def test_load_dict(self):
        assert campaign.TraceInputFile.load_dict({
            "source": "thing.py",
            "destination": "/etc",
            "permissions": False
        }) == campaign.TraceInputFile(
            source=Path("thing.py"),
            destination=Path("/etc"),
            permissions=False
        )

    def test_load_dict_no_dest(self):
        assert campaign.TraceInputFile.load_dict({
            "source": "thing.py",
        }) == campaign.TraceInputFile(
            source=Path("thing.py"),
            destination=None,
            permissions=True
        )

    def test_load_dict_dest_cwd1(self):
        assert campaign.TraceInputFile.load_dict({
            "source": "thing.py",
            "destination": "./",
        }) == campaign.TraceInputFile(
            source=Path("thing.py"),
            destination=None
        )

    def test_load_dict_dest_cwd2(self):
        assert campaign.TraceInputFile.load_dict({
            "source": "thing.py",
            "destination": ".",
        }) == campaign.TraceInputFile(
            source=Path("thing.py"),
            destination=None
        )

    @patch.object(campaign, "open", new_callable=mock_open)
    def test_check_source_relative(self, mock_file):
        fi = campaign.TraceInputFile(Path("thing.py"))
        fi.check_source(Path("/etc"))
        assert fi.source == Path("/etc/thing.py")
        mock_file.assert_called_once_with(Path("/etc/thing.py"), "r")

    @patch.object(campaign, "open", new_callable=mock_open)
    def test_check_source_abs(self, mock_file):
        fi = campaign.TraceInputFile(Path("/bin/thing.py"))
        fi.check_source(Path("/etc"))
        assert fi.source == Path("/bin/thing.py")
        mock_file.assert_called_once_with(Path("/bin/thing.py"), "r")

    @patch.object(campaign, "open", new_callable=mock_open)
    def test_check_source_rel_no_root(self, mock_file):
        fi = campaign.TraceInputFile(Path("thing.py"))
        fi.check_source()
        assert fi.source == Path("thing.py")
        mock_file.assert_called_once_with(Path("thing.py"), "r")

    def test_get_file_script(self):
        mock_source = MagicMock()
        mock_source.name = "app setup.sh"
        mock_source.stat.return_value.st_mode = 0o77755
        assert campaign.TraceInputFile(
            source=mock_source,
            destination=Path("/etc/my app config"),
            permissions=True
        ).get_file_script() == "\n".join([
            "${S2EGET} 'app setup.sh'",
            "chmod 755 'app setup.sh'",
            "mv 'app setup.sh' '/etc/my app config'",
        ])

    def test_get_file_script_set_permissions(self):
        assert campaign.TraceInputFile(
            source=Path("setup.sh"),
            permissions="124"
        ).get_file_script() == "\n".join([
            "${S2EGET} setup.sh",
            "chmod 124 setup.sh"
        ])

    def test_get_file_script_invalid_permissions_len(self):
        with pytest.raises(ValueError):
            campaign.TraceInputFile(
                source=Path("setup.sh"),
                permissions="7775"
            ).get_file_script()

    def test_get_file_script_invalid_permissions_oct(self):
        with pytest.raises(ValueError):
            campaign.TraceInputFile(
                source=Path("setup.sh"),
                permissions="78a"
            ).get_file_script()

    def test_get_file_script_simple(self):
        assert campaign.TraceInputFile(
            source=Path("/path/to/app setup.sh"),
            permissions=False
        ).get_file_script(s2e_get_var="/bin/s2eget", indent="  ") == "  /bin/s2eget 'app setup.sh'"


class TestTraceArg:

    def test_arg_type_concrete(self):
        con = campaign.TraceArg(campaign.TraceArgType.concrete)
        assert con.is_concrete

    def test_arg_type_symbolic(self):
        con = campaign.TraceArg(campaign.TraceArgType.symbolic)
        assert con.is_symbolic


class TestCampaignJsonEncoder:

    def test_enum(self):
        assert campaign.CampaignJsonEncoder().default(campaign.TraceArgType.concrete) == "concrete"

    def test_path(self):
        path = Path.cwd()
        assert campaign.CampaignJsonEncoder().default(path) == str(path)

    def test_super(self):
        with pytest.raises(TypeError):
            campaign.CampaignJsonEncoder().default(100) == "100"
