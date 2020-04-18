/* assumes the string is big enough */
void sputd(char* s, int d) {
    int counter = 0;
	if (d == 0) {
		s[counter++] = '0';
		s[counter++] = '\0';
		return;
    }
	
	if (d < 0) {
		s[counter++] = '-';
	}

	int d_abs = abs(d);
    while (d_abs != 0) {
        int number = d_abs % 10;
        s[counter++] = (char)(number + (number > 0x9 ? 55 : 48));
        d_abs /= 10;
    }
    s[counter] = '\0';
}
