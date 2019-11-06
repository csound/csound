### To build using docker file follow these steps (in local docker enviornment):
`docker build -t <name:tag> path_to_dockerfile --> Builds image from docker file`

`docker run --name <container_name> -it image -->Runs the container in iteractive mode`

`docker cp csound_test_run:/csound/Emscripten/Csound6.13.0-Web.zip . -->Exports the emscripten build back to host`


# Build Pipelines:

### Working with the Azure build pipeline: 

The **azure_pipelines.yml** file, present in main directory, defines the steps for the azure pipeline to build csound for Android and Emscripten platforms. 

You can use the variables to modify the paths and locations as per your use case. 

#### Downloading the artifacts:

In the pull request you can find the link to the build. From there you can go the azure site which details out the build steps and and at upper right side you should see the option to download artifacts.

For example,

[Azure build](https://dev.azure.com/sj6045/Csound%20builds/_build/results?buildId=52)


### Working with the github actions : 

The **csound_build.yml** present under csound/.github/workflows/ defines the steps for the github actions. 

#### Downloading the artifacts:

Github actions builds can be downloaded by going to the **Actions** tab on the csound github repo page and selecting the build you want.

For example,

[Github actions build](https://github.com/ShantanuJamble/csound/actions)


