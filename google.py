
import logging
import urllib.parse
import gdown  # You are already importing this!
# requests and alive_bar are used by your custom functions,
# but gdown handles these internally (it uses requests and has its own progress).
# If other parts of S2E rely on your specific alive_bar, you might need to adjust,
# but for the download itself, gdown is self-contained.

logger = logging.getLogger(__name__)

# CHUNK_SIZE = 1024 * 128 # gdown handles chunking
# def _save_response_content(response, destination): ... # gdown handles saving and progress
# def _download(docid, destination): ... # gdown's main function replaces this

def download(public_url, destination):
    """
    Downloads a file from a public Google Drive URL to the given destination.
    Uses the gdown library to handle potential confirmation pages.
    """
    logger.info(f"Attempting to download from Google Drive: {public_url} to {destination} using gdown.")

    # gdown can often take the full 'open?id=' URL directly.
    # Alternatively, you can extract the ID and pass it.
    # Using the ID with fuzzy=True is often robust.
    try:
        parsed_url = urllib.parse.urlparse(public_url)
        query_params = urllib.parse.parse_qs(parsed_url.query)
        file_id = query_params.get('id')

        if not file_id or not file_id[0]:
            logger.error(f"Could not extract file ID from URL: {public_url}")
            # Fallback to trying the full URL with gdown if ID extraction fails
            logger.info("Attempting download with full URL as fallback.")
            gdown.download(url=public_url, output=destination, quiet=False, fuzzy=True)
        else:
            actual_id = file_id[0]
            logger.info(f"Extracted file ID: {actual_id}. Proceeding with gdown.")
            # quiet=False will show gdown's default progress bar.
            # fuzzy=True helps gdown match the file even if the ID format is slightly off.
            gdown.download(id=actual_id, output=destination, quiet=False, fuzzy=True)

        logger.info(f"Successfully downloaded {public_url} to {destination}")
        # If the calling S2E code expects a specific return value on success, add it here.
        # For example, return True
    except Exception as e:
        logger.error(f"gdown failed to download {public_url}: {e}")
        # Propagate the error or handle as S2E expects
        raise

# Example usage (if you were to run this script standalone for testing):
# if __name__ == '__main__':
#     logging.basicConfig(level=logging.INFO)
#     test_url = "https://drive.google.com/open?id=1RltZ99RXk4dnP8XHAldVyCgUcliDTplY" # Your problematic URL
#     test_dest = "./debian-9.2.1-i386.tar.xz"
#     try:
#         download(test_url, test_dest)
#         print(f"Test download successful to {test_dest}")
#     except Exception as e:
#         print(f"Test download failed: {e}")
