# Slack Power Reporter (spr)
Slack Power Reporter - simple program which reads your battery status from sysfs and publishes change at your Slack profile.

## Recuirement

You need Slack user token to authenticate your program, therefore, you need to create "Slack App" for this.

In Slack, click

- More -> Tools -> Apps -> Open in Slack Marketplace

In browser, click

- Build -> Create New App -> From Scratch -> Create

In App Configuration page, select "OAuth and permissions", click

- Scopes -> User Token Scope -> Add an OAuth Scope -> select "user.profile:write"

Then select "Install App" and click on Install button.

After successfull installation, you'll be provided with "User OAuth Token". Keep it to use in your program.

Please refer Slack manual for additional information.

## Dependencies

No external dependencies other than libcurl. In Ubuntu 24, install it with
```
% apt install libcurl4-openssl-dev
```
## Compilation

Edit the AUTH line at the **spr.c** file, replace the token with your own one.

Compile with
```
% gcc -o spr spr.c -l curl
```
## Installation

You have to use some management program, like daemontools or systemd.

### Daemontools (runit)

Copy files from 'daemontools' directory to your service's location.<br>
Make sure **'run'** and **'log/run'** files have executable bit set.<br>
Copy **'spr'** executable to your service directory, make sure 'run' file has a proper path for it.<br>
Activate as usual with **'ln -s ../spr'**.<br>

### Systemd

Copy service definition file from 'systemd' directory to the user directory:
```
% cp systemd/spr.service ~/.config/systemd/user/
```
Make sure you've stated correct path to the **spr** executable at the **spr.service** definition file.

Tell systemd reload configuration, enable and start your service:
```
% systemctl --user daemon-reload
% systemctl --user enable spr
% systemctl --user start spr
```
## Troubleshooting

Here's command line to debug Slack API with curl invocation (use your token here):
```
% curl -q -H "Authorization: Bearer xoxp-XXXXXXXXXX-XXXXXXXXXXXXX-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" -H "Content-type: application/json; charset=utf-8" https://slack.com/api/users.profile.set -X POST -d'{"profile": {"status_text": "On Battery", "status_emoji": ":battery:", "status_expiration": 0}}'
```
 You want to see JSON output starting with **"ok":true** from server and "On Battery" status text with green battery image at your profile.
