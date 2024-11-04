## How to do a new libdivide release

Releases are semi-automated using GitHub actions:

1. Manually run the [Create draft release](https://github.com/ridiculousfish/libdivide/actions/workflows/prepare_release.yml) workflow/action.
    * Choose the branch to release from (usually ```master```) and the release type (based on [Semantic Versioning](https://semver.org/))
    * The action will do some codebase housekeeping and create a draft release:
      * Creates a new commit with updated version numbers in ```libdivide.h```, ```CMakeLists.txt```, ```library.properties```.
      * Creates a draft Git tag of format vX.Y.Z. 
2. Once the action is complete, follow the output link in the action summary to the generated draft release. E.g. ![image](https://github.com/user-attachments/assets/7e8393f7-f204-4b3a-af37-de5e187479dc)
3. Edit the generated release notes as needed & publish

Note that PRs with the ```ignore-for-release``` label are excluded from the generated release notes.
