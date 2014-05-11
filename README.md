## Solefish
This is solefish, an attempt at porting the sunfish engine to c++ from python. This is mostly for learning purposes (for myself and those who read it), but along the way maybe a usable engine will come out of it.

Link to sunfish repo: https://github.com/thomasahle/sunfish

## Status
**In it's current state it does not work.** This is a work in progress. In addition to not working I am sure most of the issues that existed in the original engine exist here as well, I have not yet tested for or fixed them.

## Development Notes
This is a straight port from the python. I matched the original program structures as best as I could in c++ without considering what would be optimal for something written in c++. Over time I plan to change that. If you find any issues or bugs in the program, please post an issue or submit a pull request. If you have any suggestions regarding code/comment style, post an issue.

## Build notes
This has been built and tested using vs2013 on win8.1 and g++ 4.8 on Debian(Wheezy) and Slackware 14.1.
TODO: Post build command.

## Things in progress (not ordered):
* Get it the point of being functionally playable.
* Cleaning up the code and increasing the amount of comments to an understandable and informative level near that of the original sunfish engine.
* I have a semi-working bitboard implementation in a different repo which I hope to integrate into this project.
* Add time controls.
* Improve move parsing. ( eg, instead of e2e4, could say e4,E4,pe4 and so on. )
* Add XBoard support.
* Add FICS support.
