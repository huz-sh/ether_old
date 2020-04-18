int num_len(int i) {
	if (i != 0) {
		/* TODO: negative numbers */
		return floor(log10(abs(i))) + 1;
	}
	else {
		return 1;
	}
}
