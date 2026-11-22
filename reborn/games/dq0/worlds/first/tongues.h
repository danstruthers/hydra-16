/*
** tongues.h - (the first world: ../../WORLDS.md) its tongues' sounds, as its history.txt's tongue lines have them
** (themegen's make.js holds the two the same), for what names things with no world's data at hand: bankrun's
** namegen.bp, and the programs between dungeons (dq0acct's town, dq0schol's scholars: on hold, the first world's
** names till they read the world's).  Each tongue (1 ...: the history's people lines' numbers) its onsets, its vowels,
** its codas (words, a space between), and a people's ending.
*/

#define NG_FIRST        6
static const char* const ng_first[NG_FIRST][4] = {
    { "kh dr gr b d th k g br z n m r t", "a u o i a u ai", "n r m k d rd rn zd l th g", "im" },
    { "v m c t s l d r h b f", "a e i o a e ae", "n r s l m d nd st", "ar" },
    { "l s th v n f m r", "ae e i ia a ei", "l n s th r", "ai" },
    { "g h r w b gr hr br th", "u o a ea", "th g rd lf n m", "ings" },
    { "k y sk g z sn kr", "i a ee", "k p x t", "ik" },
    { "s sh v z h m th", "e a i ou", "sh s th n l", "ani" }
};
