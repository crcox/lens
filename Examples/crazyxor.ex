# Crazy XOR
proc: {
  puts "You just loaded the crazy XOR file, beware!"
  setTime -i 3
}
max:2 min:0.5;

# Here is the first example.  It has two events:
name:{0 0}
freq:2.7
proc: {puts "this one's easy"}
2
[0 max:2 min: 1]
[1 max:2.5 proc:{puts "starting the second event"}]
# Only specifying inputs for the first event and targets for the second:
[0] I: 0 0
[1] T: 0;

# Here is the second example.  It has one event:
freq: 4.5
name: "0 1"
proc:{puts "example 2"}
# This means all (one) events have maxTime of 3.5:
[max:3.5]
i:1
T:1;

# Here is the third example.  It has two events with the default headers:
name:1-0
2
# Both events use inputs "1 0", but the first has no targets.
[] I: 1 0
[1] t:*;

# Here is the fourth example.  It has three events:
name: {1 1}
3
proc: {puts "This is the toughy"}
# Same inputs for all three events, no target on middle event:
[0-1 min:1.5]
I: 1 1
[0 2]
T: 0;
