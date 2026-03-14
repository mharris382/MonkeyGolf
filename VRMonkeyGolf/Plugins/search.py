import os

# CHANGE THIS to the drive or root folder you want to search
SEARCH_ROOT = "B:\\"


# Word to look for
TARGET_WORD = "close"

def find_eagle_close_folders(root):
    for dirpath, dirnames, filenames in os.walk(root):
        if "mods" in dirpath:
            print(dirpath)        # for dirname in dirnames:
        #     folder_name = dirname.lower()

        #     if TARGET_WORD in folder_name:
        #         full_path = os.path.join(dirpath, dirname)

        #         print(full_path)


if __name__ == "__main__":
    find_eagle_close_folders(SEARCH_ROOT)