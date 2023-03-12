# LayerMatNode

### Setup

#### Set environment variables

- `ARNOLD_PATH` to `C:\Program Files\Autodesk\Arnold\maya2023`

- `ARNOLD_PLUGIN_PATH` to `your plugin folder`, in this case is `/plugin` of this project
- `MTOA_TEMPLATES_PATH` the same as `ARNOLD_PLUGIN_PATH`
- Add `%ARNOLD_PATH%\bin` to your system's `PATH` variable

#### Build project

- Clone
- Generate Visual Studio project with CMake
- Build

#### Load and test plugin

- If environment variables are set properly, then Maya and Arnold will automatically load the plugin



#### Arnold for Maya development handbook

- [https://help.autodesk.com/view/ARNOL/ENU/?guid=arnold_dev_guide_plugins_html](https://help.autodesk.com/view/ARNOL/ENU/?guid=arnold_dev_guide_plugins_html)