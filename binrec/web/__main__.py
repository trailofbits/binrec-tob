if __name__ == "__main__":
    from binrec.core import init_binrec

    from .app import app

    init_binrec()

    app.run(port=8080, debug=True)
