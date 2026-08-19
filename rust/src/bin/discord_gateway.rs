use futures_util::{SinkExt, StreamExt};
use serde_json::{json, Value};
use std::env;
use std::path::Path;
use std::time::Instant;
use tokio::process::Command;
use tokio::time::{interval, Duration};
use tokio_tungstenite::{connect_async, tungstenite::Message};

async fn run_xcp(
    event: &str,
    token: &str,
    command: &str,
    channel: &str,
    message_id: &str,
    ping: u128,
    interaction_id: &str,
    interaction_token: &str,
    handler: &str,
) {
    let values = [
        ("XCP_DISCORD_EVENT", event),
        ("XCP_DISCORD_TOKEN", token),
        ("XCP_DISCORD_COMMAND", command),
        ("XCP_DISCORD_CHANNEL", channel),
        ("XCP_DISCORD_MESSAGE_ID", message_id),
        ("XCP_DISCORD_PING", &ping.to_string()),
        ("XCP_DISCORD_INTERACTION_ID", interaction_id),
        ("XCP_DISCORD_INTERACTION_TOKEN", interaction_token),
    ];
    let mut command_runner = if Path::new("./xcp").is_file() {
        Command::new("./xcp")
    } else {
        Command::new("xcp")
    };
    command_runner.arg(handler);
    for (key, value) in values {
        command_runner.env(key, value);
    }
    if let Err(error) = command_runner.status().await {
        eprintln!("xcp handler error: {error}");
    }
}

#[tokio::main]
async fn main() {
    let token = match env::args().nth(1) {
        Some(value) if !value.is_empty() => value,
        _ => {
            eprintln!("discord token is empty");
            std::process::exit(1);
        }
    };
    let handler = env::args().nth(2).unwrap_or_else(|| "discord/discord_gateway.xcp".to_string());
    let (socket, _) = match connect_async("wss://gateway.discord.gg/?v=10&encoding=json").await {
        Ok(value) => value,
        Err(error) => {
            eprintln!("discord gateway error: {error}");
            std::process::exit(1);
        }
    };
    println!("discord bot is on");
    let (mut write, mut read) = socket.split();
    let mut beat = interval(Duration::from_secs(45));
    let mut sequence: Option<Value> = None;
    let mut heartbeat_sent = Instant::now();
    let mut ping_ms = 0u128;
    loop {
        tokio::select! {
            packet = read.next() => {
                let Some(Ok(Message::Text(raw))) = packet else { break };
                let Ok(data) = serde_json::from_str::<Value>(&raw) else { continue };
                if data["op"] == 10 { let identify=json!({"op":2,"d":{"token":token,"intents":33281,"properties":{"os":"linux","browser":"xcplang","device":"xcplang"}}}); let _=write.send(Message::Text(identify.to_string().into())).await; if let Some(ms)=data["d"]["heartbeat_interval"].as_u64(){beat=interval(Duration::from_millis(ms));} }
                if data["op"] == 11 { ping_ms=heartbeat_sent.elapsed().as_millis(); }
                if data["s"].is_number(){sequence=Some(data["s"].clone());}
                if data["op"] != 0 { continue; }
                match data["t"].as_str() {
                    Some("MESSAGE_CREATE") if !data["d"]["author"]["bot"].as_bool().unwrap_or(false) => { let _=run_xcp("message",&token,data["d"]["content"].as_str().unwrap_or(""),data["d"]["channel_id"].as_str().unwrap_or(""),data["d"]["id"].as_str().unwrap_or(""),ping_ms,"","",&handler).await; }
                    Some("INTERACTION_CREATE") if data["d"]["type"] == 2 => { let name=data["d"]["data"]["name"].as_str().unwrap_or(""); let _=run_xcp("slash",&token,&format!("/{name}"),data["d"]["channel_id"].as_str().unwrap_or(""),"",ping_ms,data["d"]["id"].as_str().unwrap_or(""),data["d"]["token"].as_str().unwrap_or(""),&handler).await; }
                    _ => {}
                }
            }
            _ = beat.tick() => { heartbeat_sent=Instant::now(); let heartbeat=json!({"op":1,"d":sequence}); let _=write.send(Message::Text(heartbeat.to_string().into())).await; }
        }
    }
    println!("discord bot is off");
}
