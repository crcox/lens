# This is code that might be run in the server during parallel training.

set workingDirectory ~/Lens
set networkScript  ~/Lens/Examples/filler.in
set clientScript   client.tcl
set clientMachines {vim vim}
set synchronous    1
set batch          400
set updates        1000
set reportInterval 10
set rate           0.005
set weightFile     $workingDirectory/final.wt.gz

# Start the server
set port [startServer 9000]

# Write the customized client script
set customClientScript $workingDirectory/client[getSeed].tcl
regsub -all / $networkScript \\/ netScript
sed "s/SCRIPT/$netScript/; s/SERVER/$env(HOST)/; s/PORT/$port/" \
    $clientScript > $customClientScript

# Here we define a command for launching clients using rsh
proc launchClients {customClientScript machines} {
    global env
    set i 0
    foreach client $machines {
        puts "  launching on $client..."
	exec /usr/bin/rsh $client -n \
	    "lens -b $customClientScript > /dev/null" &
        incr i
    }
   return $i
}

# Now we use the command
puts "Launching clients..."
set numClients [launchClients $customClientScript $clientMachines]
#set numClients 2

# Load the network and training set
source $networkScript

# Now wait for the clients to connect.
puts "Waiting for $numClients clients..."
waitForClients $numClients

# Set some parameters.
setObject batchSize    $batch
setObject learningRate $rate

# Start training and wait for it to finish.
puts "Training..."
puts [trainParallel $synchronous $updates $reportInterval]

puts "Saving weights..."
saveWeights $weightFile

# Now break the barrier holding the clients so they can exit.
puts "Releasing clients..."
waitForClients
exec rm [glob $customClientScript]
puts "Ba-bye"
exit
