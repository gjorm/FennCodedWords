# FennCodedWords
Brute Force attack against the Forrest Fenn Coded Message in Armchair Treasure Hunts

In the book "Armchair Treasure Hunts" by Jenny Kile, there is an 80 numeral coded message from Forrest Fenn. In going through the steps to solve the coded message, the 80 characters are (thought) to be a transpose as the last step on the output of a Vigenere cipher with a (yet) unknown Key Phrase. 

This code uses a corpus of text gathered from online Fenn quotes, and builds a QuadGram database. These quadgrams are used to both score the usefulness of the key, as well as the plaintext it has decrypted. Additionally, instead of brute forcing through every possible character in the key, the code builds an 80 character key using quadgrams, instead of iterating through all possible k-permutations with repitition.

It may not be necessary to have an 80 character key, as Kile has hinted in her MysteriousWritings.com forums.

This uses OpenMP to speed up the attack.
