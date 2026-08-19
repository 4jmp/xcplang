# discord bot example

The example is in `tests/discordbot.xcp`. It defines two handlers:

- `on_message` answers messages that start with `!ping`.
- `on_ping_command` answers the `/ping` slash command.

Both handlers send:

```text
🏓 Pong!
Ping: <ping value>
```

The file can register any slash command with `discord_register_slashcommand(token, application_id, guild_id, name, description)`. Slash events call `on_slash_command(command, channel, interaction_id, interaction_token, ping)`, where the file decides which command to answer.

Discord Gateway event reading is handled by the native Rust `xcpgateway` binary. The `.xcp` file is the command and response layer. C++ starts the Rust process and forwards Ctrl+C. No JavaScript runtime is needed for the bot.

Build and install the gateway with `./install.sh`, or use `cargo build --release --bin xcpgateway` during development.

Keep the bot token at the end of the file:

```xcp
let token = "";
```

Never commit a real token. Use an environment variable and pass it to the host in production.
