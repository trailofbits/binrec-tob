import shlex
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Optional, Tuple

from flask import (
    Flask,
    abort,
    g,
    redirect,
    render_template,
    request,
    send_file,
    url_for,
)

from binrec.campaign import Campaign, TraceParams
from binrec.env import (
    BINREC_PROJECTS,
    campaign_filename,
    get_trace_dirs,
    project_dir,
    recovered_binary_filename,
)
from binrec.project import clear_project_trace_data, new_project

app = Flask(__name__)


@dataclass
class AppState:
    running_project: Optional[Tuple[str, subprocess.Popen]] = None
    lift_results: Dict[str, int] = field(default_factory=dict)

    def is_worker_running(self) -> bool:
        return self.running_project is not None

    def is_project_running(self, project: str) -> bool:
        return self.running_project[0] == project if self.running_project else False

    def poll(self) -> None:
        if self.running_project:
            project, worker = self.running_project
            rc = worker.poll()
            if rc is not None:
                self.lift_results[project] = rc
                self.running_project = None

    def set_running_project(self, project: str, worker: subprocess.Popen) -> None:
        self.running_project = (project, worker)
        self.lift_results.pop(project, None)


STATE = AppState()


@app.before_request
def before_request():
    g.state = STATE
    g.state.poll()


def worker_log_filename(project: str) -> Path:
    return project_dir(project) / "web_worker.log"


def load_campaign(project: str):
    filename = campaign_filename(project)
    if not filename.is_file():
        abort(404)

    try:
        campaign = Campaign.load_project(project)
    except:  # noqa: E722
        app.logger.exception("failed to load campaign for project %s", project)
        abort(404)

    return campaign


@app.get("/")
def dashboard():
    projects = sorted(
        [
            dirname.name
            for dirname in BINREC_PROJECTS.iterdir()
            if dirname.is_dir() and (dirname / "campaign.json").is_file()
        ]
    )
    return render_template("dashboard.html", projects=projects)


@app.get("/projects/<project>")
def project_details(project: str):
    campaign = load_campaign(project)
    collected_trace_count = len(get_trace_dirs(project))
    trace_count = len(campaign.traces)
    lift_result: int = g.state.lift_results.get(project)
    is_running: bool = g.state.is_project_running(project)

    if recovered_binary_filename(project).is_file():
        state = "Recovered"
        if lift_result is None:
            lift_result = 0
    elif is_running:
        state = "Recovering"
    elif lift_result:
        state = f"Recovery Failed with exit code: {lift_result}"
    elif trace_count:
        state = "Ready to recover"
    else:
        state = "New"

    return render_template(
        "project.html",
        campaign=campaign,
        project=project,
        collected_trace_count=collected_trace_count,
        trace_count=trace_count,
        state=state,
        previous_lift_result=lift_result,
        has_worker_log=worker_log_filename(project).is_file(),
        is_running=is_running,
        can_recover=not g.state.is_worker_running() and trace_count > 0,
    )


def _validate_trace_params(
    name: str, command_line: str, symbolic_arg_indicies: str
) -> TraceParams:
    try:
        args = shlex.split(command_line)
    except ValueError as error:
        raise ValueError(f"Invalid command line arguments: {error}")

    try:
        symbolic_args = [int(i) for i in symbolic_arg_indicies.split()]
    except ValueError as error:
        raise ValueError(f"Invalid symbolic argument indices: {error}")

    try:
        trace_args = TraceParams.create_trace_args(args, symbolic_args)
    except IndexError as error:
        raise ValueError(f"Invalid symbolic argument indices: {error}")

    return TraceParams(name=name, args=trace_args)


@app.route("/projects/<project>/add-trace", methods=["GET", "POST"])
def add_trace(project: str):
    campaign = load_campaign(project)
    if request.method == "POST":
        try:
            trace_params = _validate_trace_params(
                request.form["trace_name"],
                request.form["trace_args"],
                request.form["trace_symbolic_args"],
            )
        except ValueError as err:
            return render_template(
                "add-trace.html",
                project=project,
                campaign=campaign,
                trace_name=request.form["trace_name"],
                trace_args=request.form["trace_args"],
                trace_symbolic_args=request.form["trace_symbolic_args"],
                error=str(err),
            )

        campaign.traces.append(trace_params)
        campaign.save()

        return redirect(url_for("project_details", project=project))

    return render_template(
        "add-trace.html", project=project, trace_name="", trace_args=""
    )


@app.post("/projects/<project>/remove-trace/<int:trace_id>")
def remove_trace(project: str, trace_id: int):
    campaign = load_campaign(project)
    try:
        campaign.remove_trace(trace_id)
    except IndexError:
        abort(404)

    campaign.save()
    return redirect(url_for("project_details", project=project))


@app.post("/projects/<project>/clear-trace-data")
def clear_trace_data(project: str):
    if not campaign_filename(project).is_file():
        abort(404)

    clear_project_trace_data(project)
    return redirect(url_for("project_details", project=project))


@app.route("/add-project", methods=["GET", "POST"])
def add_project():
    if request.method == "POST":
        project = request.form["project"]
        binary_filename = request.form["binary_filename"]
        binary = Path(binary_filename).absolute()
        if not binary.is_file():
            return render_template(
                "add-project.html",
                project=project,
                binary_filename=binary_filename,
                error="binary does not exist",
            )

        if not project:
            project = binary.name

        if (BINREC_PROJECTS / project).is_dir():
            return render_template(
                "add-project.html",
                project=project,
                binary_filename=binary_filename,
                error=f"project already exists: {project}",
            )

        new_project(project, binary)
        return redirect(url_for("project_details", project=project))

    return render_template("add-project.html", binary_filename="")


@app.post("/projects/<project>/recover")
def start_recover_project(project: str):
    if g.state.is_worker_running():
        abort(503, "worker process is already running")

    _ = load_campaign(project)  # make sure the project/campaign exists and is valid
    log = open(worker_log_filename(project), "w")
    worker = subprocess.Popen(
        [sys.executable, "-m", "binrec.web.worker", "--verbose", "recover", project],
        stdout=log,
        stderr=log,
    )
    g.state.set_running_project(project, worker)

    return redirect(url_for("project_details", project=project))


@app.get("/projects/<project>/web_worker.log")
def download_worker_log(project: str):
    log = worker_log_filename(project)
    if not log.is_file():
        abort(404)

    return send_file(log)
