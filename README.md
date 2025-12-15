# "Dyskont" to prosta symulacja stworzona jako projekt na studia

--- 

## Deps 

### OpenSUSE Tumbleweed/Slowroll/Leap
```sh
sudo zypper install git gh gcc cmake make
sudo zypper install -t pattern devel_basis
```

### OpenSUSE MicroOS/Aeon/Kalpa
```sh
sudo transactional-update run zypper install git gh gcc cmake make && zypper install -t pattern devel_basis
```
--- 
## Build
```sh
gh repo clone Azafanatic/dyskont-sim
cd dyskont-sim
chmod +x buildnrun.sh
./buildnrun.sh
```
