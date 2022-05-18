class BinRecError(Exception):
    """
    Base exception for all BinRec errors. All errors that occurs within BinRec inherit
    from this base class.
    """


class BinRecLiftingError(BinRecError):
    """
    Exception raised when a lifting operation fails.
    """

    def __init__(self, message: str, pass_name: str, failure: str):
        self.message = message
        self.pass_name = pass_name
        self.failure = failure
        super().__init__(message, pass_name, failure)

    def __str__(self) -> str:
        return f"{self.message}: [{self.pass_name}] {self.failure}"
