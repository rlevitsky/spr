# SPR
Slack Power Reporter - simple program which reads your battery status from sysfs and publish changes at your Slack profile.

## Recuirement
You need Slack user token to authenticate your program. Please refer Slack manual. You have to grant your token with 'users.profile.set' permission to make it able to modify your Slack status.

## Dependencies
No external dependencies other than libcurl. In Ubuntu 24, install it with
% apt install libcurl4-openssl-dev

## Compilation
Edit the AUTH line, replace token with your one.
Compile with
% gcc -o spr spr.c -l curl

## Installation
You have to use some management program, like daemontools or systemd.
### Daemontools (runit)
Copy files from daemontools directory to your services' locatoin. Make sure 'run' and 'log/run' have executable bit set. Copy 'spr' executable to the same directory. Activate as usual with 'ln -s spr'.
### Systemd
Copy service definition file to the user directory:
% sudo cp systemd/spr.service /etc/systemd/user/

Make sure you've stated correct path tho the spr executable at the spr.service definition

Tell systemd reload, enable and start your service:
% systemctl --user daemon-reload
% systemctl --user enable spr
% systemctl --user start spr

## Troubleshooting
Here's command line debug curl invocation (use your token here). You want to see JSON output starting with '"ok":true' from server
% curl -q -H "Authorization: Bearer xoxp-XXXXXXXXXX-XXXXXXXXXXXXX-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" -H "Content-type: application/json; charset=utf-8" https://slack.com/api/users.profile.set -X POST -d'{"profile": {"status_text": "On Battery", "status_emoji": ":battery:", "status_expiration": 0}}'
