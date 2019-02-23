# FennCodedWords
Brute Force attack against the Forrest Fenn Coded Message in Armchair Treasure Hunts

In the book "Armchair Treasure Hunts" by Jenny Kile, there is an 80 numeral coded message from Forrest Fenn. In going through the steps to solve the coded message, the 80 characters are (thought) to be a transpose as the last step on the output of a Vigenere cipher with a (yet) unknown Key Phrase. 

This code uses a corpus of text gathered from online Fenn quotes, and builds a QuadGram database. These quadgrams are used to both score the usefulness of the key, as well as the plaintext it has decrypted. Additionally, instead of brute forcing through every possible character in the key, the code builds an 80 character key using randomly selected quadgrams from a hash table, instead of iterating through all possible k-permutations with repitition.

It may not be necessary to have an 80 character key, as Kile has hinted in her MysteriousWritings.com forums.

This has not cracked the Vigenere cipher / Final Transpose step. I have included the output file from one 25 hour compututation run, in which 32,200,000,000 attacks with an 8 core processer, hoping to happen upon the lucky key. There are 1.5 x 10^113 possible keys.

This uses OpenMP to speed up the attack.
