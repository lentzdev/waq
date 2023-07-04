info("13-04-1992 00:00:00","20-04-1992 23:59:59","2:283/512@fidonet.org","Aardbeving kennistest");
store q1;






















{
puts("\026\001\014" "Waar is volgens jou de veiligste plek tijdens een aardbeving?\n");
puts("\026\001\016" "  A " "\026\001\007" "..." "\026\001\012" " Buiten, midden op straat\n");
puts("\026\001\016" "  B " "\026\001\007" "..." "\026\001\012" " In de kelder\n");
puts("\026\001\016" "  C " "\026\001\007" "..." "\026\001\012" " Op het dak\n");
puts("\026\001\016" "  D " "\026\001\007" "..." "\026\001\012" " In een deuropening\n");
puts("\026\001\016" "  E " "\026\001\007" "..." "\026\001\012" " Onder een tafel of bed\n");
puts("\026\001\016" "  F " "\026\001\007" "..." "\026\001\012" " Maakt niet uit, is allemaal even gevaarlijk\n");
puts("\026\001\016" "  G " "\026\001\007" "..." "\026\001\012" " Geen idee\n");
puts("\n");
puts("\026\001\017" "Maak je keuze: ");
again1: puts("\026\001\003");
q1 = getc(0);
puts("\026\001\017");
if (q1 < 'A' || q1 > 'G') {
putc(q1);
puts("? Alleen A-G mogelijk, probeer opnieuw: ");
goto again1;
}
puts("\n");

puts("Bedankt voor het meedoen!\n");
puts("\026\001\016");
puts("(Binnenkort volgt de uitslag en het juiste antwoord)\n");
end();
}



