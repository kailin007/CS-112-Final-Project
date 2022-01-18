# CS-112-Final-Project
Final Project for CS112

## Dependence
Use the package manager [apt-get](https://linux.die.net/man/8/apt-get) to install Openssl and Openssl development package
```bash
sudo apt update
sudo apt upgrade
sudo apt-get install libssl-dev
```

## Configuration of Browser
Before using proxy, set up the browser first. Take Firefox as an example:
```bash
Enter "about:config" into the address bar, proceed and search:    
    * "security.enterprise_roots.enabled" -change it to be true  
    * "network.stricttransportsecurity.preloadlist" -change it to be false  

Enter "about:preferences" into the address bar:    
    * In the "General" setting, find the "Network Settings" and proceed to set up proxy  
    * In the "Privacy & Security" setting, find "Cookies and Site Data" block and click on "Clear Data" tab.  
      Tick the checkbox "Delete cookies and site data when Firefox is closed"  
    * In the "History" setting, choose "Never remember history" option
```

## Usage
```bash
make
# Flage = 1 : enable trusted proxy, Flage = 0 : disable trusted proxy.
# Blocklist : list of hostnames that proxy wants to block, separated by comma ("NA" for not blocking). e.g: "www.tufts.com,www.youtube.com" /"www.tufts.com"/"NA"
# BandwidthLimit: The bandwidth that proxy wants to limit for one client, unit: Bytes per second
./a.out port Flage Blocklist BandwidthLimit
```

## Cooperative Cache Server Configuration
Put "cache_server.txt" into the same directory as the main program.  
Format: [host:port] (square brackets are needed)  
e.g.:
```bash
[localhost:8080]
[localhost:9150]
[localhost:0909]
\n
```
Each proxy occupies a separate line.  
Put a blank line at the end of file.  


## Contributing
Jieling Cai, Kailin Zhang, Xiaoxiong Huang
