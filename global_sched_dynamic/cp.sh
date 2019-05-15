#!/bin/bash

counter=0
max=17
while [ $counter -le $max ]
do
	((counter++))
	cp bin/streaming bin/streaming$counter
done
counter=0
while [ $counter -le $max ]
do
	((counter++))
	cp bin/compute_fp bin/compute_fp$counter
done
counter=0
while [ $counter -le $max ]
do
	((counter++))
	cp bin/lg_matmul bin/lg_matmul$counter
done
counter=0
while [ $counter -le $max ]
do
	((counter++))
	cp bin/sm_matmul bin/sm_matmul$counter
done

