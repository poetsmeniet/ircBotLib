# ircBotLib
To clone this, including submodules run:
1. git clone --recursive https://github.com/poetsmeniet/ircBotLib.git
2. edit replies.txt, config.txt and channels.txt to suite your needs
3. make

Loving IRC (ah! memories!) made this a cool learning experience (C)
Resulting in the following features:
- Automated responses using external file, using regex for triggers
- External channel list file (optional)
- External application configuration
- Able to request ALL channels and connect (optionally limited) (use with care)
- Hard coded kill switch - ie; disconnect
- IRC shell (mainly for debugging)

Todo:
- Execute IRC commands
- Execute system commands
- Auth for above ;-)

The project internally has a modular setup with branched submodules tcp connection, 
hashtables, configuration parsing and generic useful tooling
