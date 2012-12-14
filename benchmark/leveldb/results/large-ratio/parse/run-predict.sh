awk '(NR > 1) {print $1, $6;}' mris_ratio_thput.dat > ratio_thput_predict.dat
