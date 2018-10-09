#!/usr/bin/python
from mininet.log import setLogLevel
from mininet.node import OVSKernelSwitch  # , KernelSwitch
from mininet.cli import CLI
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.node import CPULimitedHost
from mininet.util import pmonitor
from signal import SIGINT
from time import time
import os
import os
class SingleSwitchTopo(Topo):
    #Build the topology here.
    #We will use a simple star topology for this assignment.
    #If you want to build more complex topologies for testing, 
    #check examples in mininet/examples from the home directory of this VM.
    #Single switch connected to 4 hosts.
	def __init__(self, n=4, **opts):
		Topo.__init__(self, **opts)
        #add hosts, switch, and links here
        #use self.addHost() to add new nodes
        #and self.addLink() to add links between switches
		switch = self.addSwitch('s1')
	 	#for h in range(n):
		LeftClient = self.addHost('hl')
        #self.addLink(host, switch)
		self.addLink(LeftClient , switch, bw=100, loss=10, delay='0ms')
		RightClient = self.addHost('hr')
		self.addLink(RightClient , switch, bw=100, loss=0, delay='0ms')
		TopClient = self.addHost('ht')
		self.addLink(TopClient , switch, bw=100, loss=0, delay='0ms')
		BotClient = self.addHost('hb')
		self.addLink(BotClient , switch, bw=100, loss=0, delay='0ms')
		

def test():
	os.system('mn -c')
    #os.system code goes here
	os.system('sysctl -w net.ipv4.tcp_congestion_control=cubic')
	os.system('sysctl -w net.ipv4.tcp_sack=0')
	#os.system('sysctl -w net.ipv4.tcp_reordering=31')
    #Note that the configuration in this python script will affect mininet.
    #Therefore, please make sure that you restore the configuration to the default value.
	switch = OVSKernelSwitch
	topo = SingleSwitchTopo(0)
	network = Mininet(topo=topo, host=CPULimitedHost, link=TCLink, switch = switch)
	network.start()
    #CLI(network)
    #If you want to implement your assignment on the command line, use CLI(network). 
    #CLI description can be found at
    #   http://mininet.org/walkthrough/#part-3-mininet-command-line-interface-cli-commands
    #Or use popen features to trigger the system calls on specific hosts.
    #popen description can be found at
    #   https://docs.python.org/2/library/subprocess.html
	#hl = network.hosts[0]
	#hr = network.hosts[2]

	hl = network.get('hl')
	hr = network.get('hr')
	#ht = network.get('ht')
	#print hl.name
	#print hr.name
	
	popens={}
	hl.popen("iperf -s")
	popens[hr] = hr.popen("iperf -c "+  hl.IP()+ " -t 120")
	#hl.popen("iperf -s -u")
	#popens[ht] = ht.popen("iperf -c "+ hl.IP() + " -t 120" + " -u -b 20m")
	print "Monitoring output for"
	for h, line in pmonitor( popens, timeoutms=5000 ):
		if h:
			print '%s: %s' % ( h.name, line )
	

		

    #output =network.hosts[0].popen("iperf -s")
    #print output
    #output =network.hosts[1].popen("iperf -c 10.0.0.2")
    #print output
    #popens = {}
	
	network.stop()
if __name__ == '__main__':
    setLogLevel( 'info' )
    test()
