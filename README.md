Completely functioning Cod4x vote plugin.

To build the plugin on linux, make sure you have the ```build-essential```, ```cmake``` and ```git``` installed on your machine:
```
sudo apt-get install build-essential cmake git
```

Download the repo on your local machine:
```
git clone https://github.com/user186d45/Cod4x_Vote_Plugin.git
```

Then ```cd``` into the repo and create a ```build``` directory and build the plugin inside that directory ( you can just copy paste the following and it will compile for you ):
```
mkdir build; cd build; cmake ..; make -j$(nproc); cd ..;
```

One line download and build command:
```
sudo apt-get install build-essential cmake git && git clone https://github.com/user186d45/Cod4x_Vote_Plugin.git && cd Cod4x_Vote_Plugin && mkdir build; cd build; cmake ..; make -j$(nproc); cd ..;
```

If the compilation process be successful the plugin will be present with a ```.so``` prefix at your build directory, you shall copy it into the following directory of the game in order to load it:
``` main_shared/plugins ``` if the directory is not present, just create it. Load the plugin with the ```loadplugin``` command on the server console:
```
loadplugin Cod4x_Vote_Plugin
```

Please report any issues so i can fix them! ❤️
