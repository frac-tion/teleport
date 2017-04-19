# Teleport
Teleport is a native GTK3 app to effortlessly share files on the local network.

![Teleport Mockup](docs/mvp-mockup.png)

It's our answer to the question

> Why is the easiest way to move a file between two computers in the same room sending it to a server in another country and retrieving it from there?

Teleport is designed to be a replacement for using USB keys or emailing stuff to yourself just so you have them on another device on your desk. The main user interface on the receiver's side are notifications:

![Teleport Notifications Mockup](docs/notifications.png)

## Roadmap
We are currently working on an MVP for a native GNOME app that only sends files, but longer term we are interested in doing things like:
* drag & drop files to send them
* sending text snippets
* file transfer progress bars
* settings dialog
* file encryption
* native Android/iOS/macOS/Windows apps

## Build
```
  cd src
  make
```

## License
AGPLv3, because freeeeeeedom
