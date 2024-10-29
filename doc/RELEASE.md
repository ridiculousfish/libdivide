## How to do a new libdivide release

Releases are semi-automated using GitHub actions:

1. Manually run the [Create draft release](https://github.com/ridiculousfish/libdivide/actions/workflows/prepare_release.yml) workflow/action.
    * This does some codebase housekeeping and creates a draft release.
    * This updates the version numbers in ```libdivide.h```, ```CMakeLists.txt```, ... and creates a new commit.
    * This creates a new git tag of format vX.Y.Z. 
2. Follow the output link in the action summary to the generated draft release. E.g. ![image](https://github.com/user-attachments/assets/7e8393f7-f204-4b3a-af37-de5e187479dc)
3. Edit the generated release notes as needed & publish

Note that PRs with the ```ignore-for-release``` label are excluded from the generated release notes.
