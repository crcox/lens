# This is a template for a script that can be run by a client for parallel
# training.  You will need to replace the words in all caps (possibly with
# sed to produce the actual script.

source SCRIPT
startClient SERVER PORT
puts "Ready to go..."
waitForClients
puts "Gee, that didn't hurt a bit."
exit
