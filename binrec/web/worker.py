import logging
import sys

from binrec.campaign import Campaign
from binrec.lift import lift_trace
from binrec.merge import merge_traces
from binrec.project import run_campaign

logger = logging.getLogger("binrec.web.worker")


def recover_from_campaign(project: str) -> None:
    """
    Recover a project from a campaign. This method runs each campaign trace, merges the
    traces, and then lifts the recovered binary.
    """
    logger.debug("loading campaign")
    try:
        campaign = Campaign.load_project(project)
    except:  # noqa: E722
        logger.exception("failed to load campaign for project: %s", project)
        sys.exit(-1)

    logger.debug("running campaign")
    try:
        run_campaign(campaign)
    except:  # noqa: E722
        logger.exception("failed to run campaign for project: %s", project)
        sys.exit(-1)

    logger.debug("merging collected traces")
    try:
        merge_traces(args.project)
    except:  # noqa: E722
        logger.exception("failed to merge traces for project: %s", project)
        sys.exit(-1)

    logger.debug("lifting recovered binary")
    try:
        lift_trace(args.project)
    except:  # noqa: E722
        logger.exception("failed to lift recovered binary for project: %s", project)
        sys.exit(-1)

    logger.info("recovered binary successfully lifted")
    sys.exit(0)


if __name__ == "__main__":
    import argparse

    from binrec.core import init_binrec

    init_binrec()

    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", action="count", help="verbose log output")

    subparsers = parser.add_subparsers(dest="current_parser")

    recover_action = subparsers.add_parser("recover")
    recover_action.add_argument("project", help="project name")

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger("binrec").setLevel(logging.DEBUG)
        if args.verbose > 1:
            from binrec.audit import enable_python_audit_log

            enable_python_audit_log()

    if args.current_parser == "recover":
        recover_from_campaign(args.project)
    else:
        parser.print_help()
        sys.exit(1)
