# FennCodedWords
Brute Force attack against the Forrest Fenn Coded Message in Armchair Treasure Hunts

In the book "Armchair Treasure Hunts" by Jenny Kile, there is an 80 numeral coded message from Forrest Fenn. In going through the steps to solve the coded message, the 80 characters are (thought) to be a transpose as the last step on the output of a Vigenere cipher with a (yet) unknown Key Phrase. 

This code uses a corpus of text gathered from online Fenn quotes, and builds a QuadGram database. These quadgrams, instead of brute forcing through every possible character in the key, build a 52 character key using randomly selected quadgrams from a hash table, instead of iterating through all possible k-permutations with repitition.

This has not cracked the Vigenere cipher / Final Transpose step. I have included the output file from one 25 hour compututation run, with 32,200,000,000 attacks with an 8 core processer, hoping to happen upon the lucky key. There are 3.7 x 10^73 possible keys.

This uses OpenMP to speed up the attack.

2019-04-06 - In recent update from Kile, the key is hinted at being 52 characters in length. This code has been updated to reflect that. Also this code no longer uses Quadgrams to measure fitness (it still uses them to generate the key), and now uses the Chi Squared Test, which is faster and actually yields better results. Instead of using a larger natural English letter frequency, this code calculates the Fenn letter frequency from the Fenn corpus.txt file.

Line 170 of main.cpp contains the value that limits the number of iterations to run. It is currently set to 30000000 to allow for testing.
