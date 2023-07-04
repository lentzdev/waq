info("01-11-1993 00:00:00","31-01-1994 23:59:59","2:283/512@fidonet.org","2e Kamer Verkiezingspeiling - statistiek");
store stem;
intern res, count, tmp;

intern stem_cda, stem_pvda, stem_vvd, stem_d66, stem_gl, stem_kr, stem_sp, stem_cd, stem_blanko;






















{
puts("\014");
puts("\026\001\017" "Tussentijdse cijfers van de 2e kamer verkiezingspeiling....\n");
puts("\026\001\017" "(De uiteindelijke statistiek zal uitgebreider zijn)\n");
puts("\n");

if (read(0) < 0) {
puts("Kan de antwoordfile niet openen.\n");
quit();
}

count = 0;
stem_cda = stem_pvda = stem_vvd = stem_d66 = 0;
stem_gl = stem_kr = stem_sp = stem_cd = stem_blanko = 0;

for (res = read(0); res > 0; res = read(1)) {
puts("#"); puti(++count); puts("\r");
if      (stem == 1) stem_cda++;
else if (stem == 2) stem_pvda++;
else if (stem == 3) stem_vvd++;
else if (stem == 4) stem_d66++;
else if (stem == 5) stem_gl++;
else if (stem == 6) stem_kr++;
else if (stem == 7) stem_sp++;
else if (stem == 8) stem_cd++;
else if (stem == 9) stem_blanko++;
}

if (res < 0) {
puts("Fout bij het lezen van de antwoordfile.\n");
quit();
}
read(-1);

puts("\026\001\016"   "CDA           " "\026\001\007" "..." "\026\001\012" " "); tmp = stem_cda; call i3;
puts("\026\001\016" "\nPvdA          " "\026\001\007" "..." "\026\001\012" " "); tmp = stem_pvda; call i3;
puts("\026\001\016" "\nVVD           " "\026\001\007" "..." "\026\001\012" " "); tmp = stem_vvd; call i3;
puts("\026\001\016" "\nD'66          " "\026\001\007" "..." "\026\001\012" " "); tmp = stem_d66; call i3;
puts("\026\001\016" "\nGroen Links   " "\026\001\007" "..." "\026\001\012" " "); tmp = stem_gl; call i3;
puts("\026\001\016" "\nKlein Rechts  " "\026\001\007" "..." "\026\001\012" " "); tmp = stem_kr; call i3;
puts("\026\001\016" "\nSoc.Partij    " "\026\001\007" "..." "\026\001\012" " "); tmp = stem_sp; call i3;
puts("\026\001\016" "\nCD            " "\026\001\007" "..." "\026\001\012" " "); tmp = stem_cd; call i3;
puts("\026\001\016" "\nBlanko stem   " "\026\001\007" "..." "\026\001\012" " "); tmp = stem_blanko; call i3;
puts("\026\001\003"   "\n=====================");
puts("\026\001\016" "\nStemmentotaal " "\026\001\007" "..." "\026\001\012" " "); tmp = count; call i3;
puts("\n\n");

quit();

i3:     if      (tmp <  10) puts("  ");
else if (tmp < 100) putc(' ');
puti(tmp);
return;
}



