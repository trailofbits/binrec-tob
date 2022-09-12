if __name__ == "__main__":
    from binrec.core import init_binrec

    from .app import app, setup_web_app

    init_binrec()

    setup_web_app()

    app.run(port=8080, debug=True)
