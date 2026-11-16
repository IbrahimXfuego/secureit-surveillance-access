# Publish your own public repository

These steps publish the **source code**, not a live Internet camera or the local Node gateway.

1. Create/sign in to your GitHub account.
2. Choose **New repository**, name it `secureit-surveillance-access`, select **Public**, and create it empty (do not initialize a README/license if importing this project).
3. Confirm contributors and the license with the original team. Keep the source attribution. Add a license only when you have the necessary rights.
4. Add photos to `docs/media/` and link them from the README when available. Do not upload tokens, `.env`, `config.h`, private chat screenshots, local logs or private network details.
5. If using the source ZIP, extract it and open the folder in VS Code. In its terminal:

```sh
git init
git add .
git status --short
```

Review the staged file names. `.env` and `config.h` must not be listed. Then:

```sh
git commit -m "Organize SecureIT firmware, dashboard and wiring"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/secureit-surveillance-access.git
git push -u origin main
```

Use GitHub's normal sign-in/token flow or GitHub Desktop; account passwords are not accepted for Git HTTPS pushes. Do not put tokens into repository files or remote URLs.

If you cloned the original repository instead of extracting the ZIP, preserve that history: rename its existing remote to `upstream`, then add your own repository as `origin`. Check existing remotes with `git remote -v` first. Do not push changes to the source owner's repository unless explicitly intended.

6. Open your public repository. Verify README rendering, wiring image, files and credits. Set a concise description and topics such as `esp32-cam`, `esp8266`, `telegram-bot`, `iot`, `access-control` and `hackathon`.
7. Add this repository's real URL to the portfolio project data. The portfolio's GitHub button should then appear.
8. Future updates: edit files, review `git diff`, then `git add`, `git commit` and `git push`.

The local gateway needs Node.js on a running host and network access to the boards. GitHub Pages cannot run that Node server; it can host a separate static demo or your portfolio.
