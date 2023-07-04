info("01-11-1993 00:00:00","31-12-1993 23:59:59","2:283/512@fidonet.org","2e Kamer Verkiezingspeiling");
store stem;






















{
puts("\014");
puts("\026\001\017" "De WAQ software is zodanig ontworpen dat de user absoluut anoniem blijft\n");
puts("\026\001\017" "bij het beantwoorden van vragen. Zelfs de sysop kan niet spieken.\n");
puts("\n");
puts("\026\001\014" "Als er nu 2e-kamer verkiezingen zouden worden gehouden,\n");
puts("\026\001\014" "op welke partij zou je dan stemmen? (N.B. blanko mag ook!)\n");
puts("\026\001\016" "  1 " "\026\001\007" "..." "\026\001\012" " CDA\n");
puts("\026\001\016" "  2 " "\026\001\007" "..." "\026\001\012" " PvdA\n");
puts("\026\001\016" "  3 " "\026\001\007" "..." "\026\001\012" " VVD\n");
puts("\026\001\016" "  4 " "\026\001\007" "..." "\026\001\012" " D'66\n");
puts("\026\001\016" "  5 " "\026\001\007" "..." "\026\001\012" " Groen Links\n");
puts("\026\001\016" "  6 " "\026\001\007" "..." "\026\001\012" " Klein Rechts\n");
puts("\026\001\016" "  7 " "\026\001\007" "..." "\026\001\012" " Socialistische Partij\n");
puts("\026\001\016" "  8 " "\026\001\007" "..." "\026\001\012" " CD\n");
puts("\026\001\016" "  9 " "\026\001\007" "..." "\026\001\012" " Blanko stem\n");
puts("\n");
puts("\026\001\017" "Maak je keuze: ");
again_stem:
puts("\026\001\003");
stem = geti(0);
puts("\026\001\017");
if (stem < 1 || stem > 9) {
puti(stem);
puts("? Alleen 1-9 mogelijk, probeer opnieuw: ");
goto again_stem;
}
puts("\n");

puts("\014");
puts("Bedankt voor het meedoen!\n");
puts("\026\001\016");
puts("(De lopende uitkomsten zie je verderop in het BBS!)\n");
end();
}



