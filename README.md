# Welcome to EVE LogLite!

-------------------

LogLite is a client log viewer developed by [CCP Games] (https://www.ccpgames.com/) for their award winning MMO [EVE Online] (http://www.eveonline.com/).


# How to build
The project is build using CMake.

## Configuring
Configuration is best managed using the presets included in the project. To see the list of available presets, run
the following from the project root:

`cmake --list-presets`

To configure your project with a given preset, run the following from the project root:

`cmake --preset [preset]`

This will build using the default generator as configured by the `CMAKE_GENERATOR` environment variable on the host
machine. The generator can also be explicitly provided during configuration:

`cmake -G [generator] --preset [preset]`

For typical use cases, we recommend the `Ninja Multi-Config` generator.

## Building
After configuration is complete, run the following:

`cmake --build [BUILD_DIRECTORY]`

## Dependencies

External dependencies are managed using Microsofts VCPKG package manager.

For reference, consult the following sources:

* [VCPKG Common Usage Guide](https://ccpgames.atlassian.net/wiki/spaces/TC/pages/1140359350/Common+VCPKG+Usage+Guides)
* [VCPKG Migration Guide](https://ccpgames.atlassian.net/wiki/spaces/TC/pages/1104052275/Guide+to+Migrating+Carbon+Components+to+VCPKG)
* [VCPKG ocumentation](https://learn.microsoft.com/en-us/vcpkg/)

# CI Pipeline Documentation

This pipeline automates handling open pull requests (PRs) when new commits are pushed to the repository.

[Please read this guide on setting up your prject on teamcity](https://ccpgames.atlassian.net/wiki/spaces/TC/pages/1546387638/Setting+up+a+new+TeamCity+project+based+on+the+Kotlin+Configuration+in+Carbon+Template)


---

## **How It Works**

1. **Trigger on Commit** (this is setup in the VCS root on teamcity, not by the Kotlin configuratoin in the `.teamcity` folder)
    - The pipeline runs automatically every time a commit is pushed to the main branch or when any tag has been created.
    - specifically it monitors `refs/heads/main` and `refs/tags/*` for changes.

2. **Check for Open PRs**
    - It checks if the committed branch is the **target branch** of any open PRs.

3. **Filter PRs**
    - The pipeline identifies and sorts PRs into two categories:
        - **Domestic PRs**: Source branch belongs to the same repository.
        - **International PRs**: Source branch comes from a forked repository.

4. **Dynamic Builds**
    - For each PR, the pipeline creates a separate build step using GitHub Actions' **matrix logic** to handle PRs in parallel.

5. **Merge Logic**
    - Merges the latest changes from the **target branch** (committed branch) into the **source branch** of each PR.
    - Pushes the updated source branch back:
        - Domestic PRs: Push to the same repo.
        - International PRs: Push to the forked repo.

6. **Conflict Handling**
    - If there are merge conflicts that require manual resolution, the specific PR build step fails.

---

## **Notes**

- The pipeline is dynamic and scales with the number of open PRs.
- Conflicts must be resolved manually, and affected builds will display the failure in the CI logs.
- Parallel builds ensure efficient processing of multiple PRs.

---

This setup ensures all PRs remain up-to-date with the latest changes from the target branch.


## License

-------------------

All files in this project are under the [LICENSE](LICENSE) license unless otherwise stated in the file or by a dependency's license file.


## Author

-------------------

Filipp Pavlov - Team TriLambda

