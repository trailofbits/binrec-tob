from flask import Flask, request, escape, render_template
import subprocess

app = Flask(__name__)

DUMMY_PROJS = ["eq", "ls", "bc"]
DUMMY_TRACES = ["T1", "T2", "T3"]


def list_projects():
    """
    try:
        projects =  subprocess.check_output(["just", "list-projects"], stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        raise RuntimeError("command '{}' return with error (code {}): {}".format(e.cmd, e.returncode, e.output))
    """
    projects = DUMMY_PROJS
    return [{"name": str(project)} for project in projects]


@app.route("/")
def index():
    newproj = request.args.get("projname", "")
    return render_template("index.html", data=list_projects())


@app.route("/project", methods=["GET", "POST"])
def project():
    project = request.form.get("project_select")
    trace = request.args.get("toe", "")
    return (render_template("project.html", select=project, traces=DUMMY_TRACES) + trace)
