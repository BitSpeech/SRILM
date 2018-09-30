#!/usr/local/bin/gawk -f
#
# combine-rover-controls --
#	combined several rover control files for system combination
#	(may be used recursively)
#
# $Header: /home/srilm/CVS/srilm/utils/src/combine-rover-controls.gawk,v 1.2 2005/07/05 18:20:54 stolcke Exp $
#

function process_rover_control(file, weight) {

	dir = file;
	sub("/[^/]*$", "", dir);
	if (file == dir) {
		dir = "";
	}

	while ((status = (getline < file)) > 0) {
		# deal with relative directories in rover-control file:
		# prepend rover-control directory path
		if ($1 !~ /^\// && dir != "") {
		    $1 = dir "/" $1;
		}

		if ($3 != "+") {
		    # handle missing lmw and wtw and system weights
		    if ($2 == "") $2 = 8;
		    if ($3 == "") $3 = 0;
		    if ($4 == "") $4 = 1;

		    # divide system weight by total number of input files
		    $4 = $4 * weight;
		}

		print $0;
	}

	if (status < 0) {
		print file ": " ERRNO > "/dev/stderr";
		exit(1);
	}
	close(file);

	return;
}

BEGIN {
	arg_offset = 0;
	ninputs = ARGC - 1;

	if (ARGV[1] ~ /^lambda=/) {
	    lambda = substr(ARGV[1], 8);
	    ninputs -= 1;
	    arg_offset += 1;
	}

	if (ninputs < 1) {
	    print "usage: " ARGV[0] " [lambda=WEIGHTS] ROVER-CTRL1 ROVER-CTRL2 ..." \
			    >> "/dev/stderr";
	    exit(2);
	}

        # initialize priors from lambdas
        nlambdas = split(lambda, lambdas);
        lambda_sum = 0.0;
        for (i = 1; i <= nlambdas; i ++) {
                lambda_sum += lambdas[i];
        }
        # fill in the missing lambdas with uniform values
        for (i = nlambdas + 1; i <= ninputs; i ++) {
                lambdas[i] = (1 - lambda_sum)/(ninputs - nlambdas);
        }

	for (i = 1; i <= ninputs; i ++) {
	    process_rover_control(ARGV[arg_offset + i], lambdas[i]);
	}

	exit(0);
}

