#!/usr/local/bin/gawk -f
#
# compute-best-rover-mix --
#	Compute the best mixture weight for combining multiple sausages
#
# usage: compute-best-rover-mix [lambda="l1 l2 ..."] [precision=p] nbest-rover-ref-posteriors-output
#j
# where the input is the output of nbest-rover -write-ref-posteriors .
# li are initial guesses at the mixture weights, and p is the
# precision with which the best lambda vector is to be found.
#
# $Header: /home/srilm/devel/utils/src/RCS/compute-best-rover-mix.gawk,v 1.2 2004/11/02 02:00:35 stolcke Exp $
#
BEGIN {
	verbose = 0;

	lambda = "0.5";
	precision = 0.001;
	M_LN10 = 2.30258509299404568402;	# from <math.h>

	logINF = -320;

	zero_probs = 0;
}
function abs(x) {
	return (x < 0) ? -x : x;
}
function log10(x) {
	return log(x) / M_LN10;
}
function exp10(x) {
	if (x < logINF) {
		return 0;
	} else {
		return exp(x * M_LN10);
	}
}
function addlogs(x,y) {
    if (x<y) {
	temp = x; x = y; y = temp;
    }
    return x + log10(1 + exp10(y - x));
}

function print_vector(x, n) {
	result = "(" x[1];
	for (k = 2; k <= n; k++) {
		result = result " " x[k];
	}
	return result ")"
}

{
	nfiles = NF - 4;

	if ($4 == 0) {
		zero_probs ++;
	} else {
		sample_no ++;

		for (i = 1; i <= nfiles; i++) {
			samples[i " " sample_no] = $(i + 4);
		}
	}
}
	
END {
	last_prior = 0.0;

	# initialize priors from lambdas
	nlambdas = split(lambda, lambdas);
	lambda_sum = 0.0;
	for (i = 1; i <= nlambdas; i ++) {
		priors[i] = lambdas[i];
		lambda_sum += lambdas[i];
	}
	# fill in the missing lambdas
	for (i = nlambdas + 1; i <= nfiles; i ++) {
		priors[i] = (1 - lambda_sum)/(nfiles - nlambdas);
	}

	iter = 0;
	have_converged = 0;
	while (!have_converged) {
	    iter ++;

	    num_words = 0;
	    delete post_totals;
	    log_like = 0;

	    for (j = 1; j <= sample_no; j ++) {

		all_inf = 1;
		for (i = 1; i <= nfiles; i ++) {
			sample = log10(samples[i " " j]);
			logpost[i] = log10(priors[i]) + sample;
			all_inf = all_inf && (sample == logINF);
			if (i == 1) {
				logsum = logpost[i];
			} else {
				logsum = addlogs(logsum, logpost[i]);
			}
		}

		# skip OOV words
		if (all_inf) {
			continue;
		}

		num_words ++;
		log_like += logsum;

		for (i = 1; i <= nfiles; i ++) {
			post_totals[i] += exp10(logpost[i] - logsum);
		}
	    }
	    printf "iteration %d, lambda = %s, ppl = %g\n", \
		    iter, print_vector(priors, nfiles), \
		    exp10(-log_like/num_words) >> "/dev/stderr";
	    fflush();

	
	    have_converged = 1;
	    for (i = 1; i <= nfiles; i ++) {
		last_prior = priors[i];
		priors[i] = post_totals[i]/num_words;

		if (abs(last_prior - priors[i]) > precision) {
			have_converged = 0;
		}
	    }
	}

	printf "%d alignment positions, best lambda %s\n", 
			num_words, print_vector(priors, nfiles);
}
