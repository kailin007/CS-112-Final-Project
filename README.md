# CS-112-Final-Project
Final Project for CS112

## Dependence
Use the package manager [apt-get](https://linux.die.net/man/8/apt-get) to install Openssl and Openssl development package
```bash
sudo apt update
sudo apt upgrade
sudo apt-get install libssl-dev
```

## Configuration of browser
Before using proxy, set up the browser first. Take Firefox as an example:

Enter "about:config" into the address bar, proceed and search:    
>>>>>>1. "security.enterprise_roots.enabled" -change it to be true  
>>>>>>2. "network.stricttransportsecurity.preloadlist" -change it to be false  

Enter "about:preferences" into the address bar:    
>>>>>>1. In the "General" setting, find the "Network Settings" and proceed to set up proxy  
>>>>>>2. In the "Privacy & Security" setting, find "Cookies and Site Data" block and click on "Clear Data" tab.  
>>>>>>>>>Tick the checkbox "Delete cookies and site data when Firefox is closed"  
>>>>>>3. In the "History" setting, select "Never remember history" option

## Usage
```bash
make
# Flage = 1 : enable trusted proxy, Flage = 0 : disable trusted proxy.
# Blocklist : list of hostnames that proxy wants to block, separated by comma. e.g: www.tufts.com,www.youtube.com
./a.out port Flage Blocklist
```

## Contributing
Jieling Cai, Kailin Zhang, Xiaoxiong Huang
