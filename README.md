# Waifuteer 0.0.1
$${\text{\color{red}\Large THIS IS PRE-ALPHA SOFTWARE}}$$

It is possible that you could build and use this software yourself for streaming activities; and stream as kantan-chan (or a modification of her) you are brave; i don't feel confident in this software and lots of things and documentation still need work. 

The format for models is expected to still change in non-backwards compatible ways in new versions.

## TODO
- [ ] make bogeygirl/gun.hpp compile with gcc (instead of just clang)
- [ ] thus, allowing windows builds
- [ ] let me use multiple .pup files handle different expressions
- [ ] smooth low framerate tracking
- [ ] add blink tracking
- [ ] rewrite parsering
- [ ] many other bug fixes

## wishlist
- [ ] documentation for puppet makers
- [ ] a user friendly interface???
- [ ] make my own face tracking system. i really appreciate what OpenSeeFace has done, but i heubristicly think that maybe i can make my own that is better. ~~maybe even free the vtuber communitee from having to buy apple phones~~. Also that i would like to be rid of having a python dependency for tracking. 

## Usage

```
make
```

Then if incredible luck it builds successfully on your machine, you can launch waifuteer with

```
./waifuteer
```

Which starts Waifuteer displaying kantan-chan. However, she may render off-screen. You need to run [OpenSeeFace](https://github.com/emilianavt/OpenSeeFace) to supply tracking info.

## History

Waifuteer originally was an application which could only render me, [shiffi meekling](https://meekling.neocities.org/). i started a major rewrite/update because i've wanted to make it software that other people can use too. In the past, i referred to this version of Waifuteer for other people too as Waifuteer 2. However, since this is pre-alpha software, that is confusing and thus i, henceforth, will refer to that meekling-only version of Waifuteer as Waifuteer primordial. 

## FAQ

### woah, this repository is messy.
yeah. you're right. this has been a solo project, so i have just been leaving partially-written or unused code in the master branch. And in the mean time, i think i might just keep doing that.

### Can I stream as kantan-chan, even commercially?
yep, she is public domain so you can do whatever you want with her.  You could even sell kantan-chan merch without sharing any revenue with me. Sell merch of her, even if you are a megacorp. You are encouraged to use her as a base for creating your own vtuber model, and you would retain full rights to your new model.


### in `vec.h`, you use "towards" and "towords". That is a very bad naming scheme for two different methods.

You are correct for normal professional projects. However, it makes me, the solo unpaid developer giggle and thus, it is staying in.

### why did you write your own worse parsing library instead of using boost::spirit or something?

i tried using boost::spirit but i found the error messages too confusing. My library also has confusing error messages, but i wrote it, so i am more likely to understand said error messages. i hope to write a new better parsing libriary, but i want to wait until c++26 comes out and we get reflection support.

### why don't you use complex meshes which are distorted to create animations like Live2D uses?

1. i would need to make graphical tools to have a hope of creating vtubers then. No basic vtubers defined in 118 lines on text config.
2. Art reflects the tools which were used to create it. If Waifuteer uses a different strategy to animate then Live2D, then vtubers created with Waifuteer will look different. Hence, Waifuteer can maybe fill a niche even if it never reaches feature parity with the Live2D + VtuberStudio stack.

### Live 2D isn't the software's real name. That is the name of the company. It is called Live2D cubism.

I know but everyone in English vtubing calls it Live2D.

### Why don't you use cmake like everyone else?
cmake seemed complicated when i first started this project, and now i have grown sentimentally attached to my Makefile based-build system. cmake is still required for building some of the dependencies in libs.

### why are there jokes in the source code? Even a boobs joke.
it makes me happy. you can improve this code base by adding more.

### why don't you capitalize "i" in the pronoun "I"? 
i think it is cuter. \\(^-^)/

### how do you feel about pull requests?
oh i would be delighted if i was sent some. However, there is some risk that i will reject them if they are adding features 

### as anyone actually asked these frequently asked questions?
no.

### Are you happy that I read all these questions anyway?
yes yes ^~^ thank you. ありがとうございます✨

### you can speak japanese?
nooo.... though i am studying it a bit
