proc gatherStatistics {} {
  global Class
  set units [getObj map.numUnits]
  # Initialize the table of unit/digit counts
  repeat i $units {
    repeat d 10 {
      set ClassProb($i.$d) 0
    }
  }
  # For each example
  repeat e [getObj trainingSet.numExamples] {
    doExample $e
    # This is the correct digit label
    set d [getObj trainingSet.currentExample.name]
    # Increment the counts of active units
    repeat i $units {
      if {[getObj map:$i.output] > 0} {incr ClassProb($i.$d)}
    }
    if {$e > 0 && ($e % 100) == 0} {puts $e; update}
  }
  # Find the most common class for each unit
  repeat i $units {
    set max 0
    repeat d 10 {
      if {$ClassProb($i.$d) > $max} {set max $ClassProb($i.$d); set c $d}
    }
    set Class($i) $c
  }
}

# This assumes the example has already been run and returns the classification.
proc classify {} {
  global Class
  set max 0
  repeat i [getObj map.numUnits] {
    set out [getObj map:$i.output]
    if {$out > $max} {set max $out; set maxUnit $i}
  }
  return $Class($maxUnit)
}

# This tests the classifier on all examples in the specified set.
proc testClassifier {{set test}} {
  set correct 0
  set total [getObj $set.numExamples]
  repeat e $total {
    doExample $e -s $set
    set d [getObj $set.currentExample.name]
    set c [classify]
    if {$d == $c} {incr correct}
    if {$e > 0 && ($e % 100) == 0} {puts $e; update}
  }
  puts [format "%d/%d (%.2f%%) Correct" $correct $total \
	    [expr (100.0 * $correct) / $total]]
}
