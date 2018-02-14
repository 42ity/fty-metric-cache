# fty-metric-cache

Agent fty-metric-cache serves as in-memory cache of most recent metrics.

## How to build

To build fty-metric-cache project run:

```bash
./autogen.sh
./configure
make
make check # to run self-test
```

## How to run

To run fty-metric-cache project:

* from within the source tree, run:

```bash
./src/fty-metric-cache
```

For the other options available, refer to the manual page of fty-metric-cache

* from an installed base, using systemd, run:

```bash
systemctl start fty-metric-cache
```

Agent also has a command line interface fty-metric-cache-cli.

For further information, refer to the manual page of fty-metric-cache-cli.

### Configuration file

Configuration file - fty-metric-cache.cfg - is currently ignored.

Agent reads environment variable BIOS\_LOG\_LEVEL to set verbosity level.

Agent has a state file stored in /var/lib/fty/fty-metric-cache/state\_file.

## Architecture

### Overview

fty-metric-cache has 1 actor:

* fty-metric-cache-server: main actor

It also has one built-in timer, which runs every 30 seconds and deletes outdated metrics from the cache.

## Protocols

### Published metrics

Agent doesn't publish any metrics.

### Published alerts

Agent doesn't publish any alerts.

### Mailbox requests

Agent fty-metric-cache can be requested for:

* listing devices with (some) metrics available

* getting metrics for specified device

#### List devices with (some) metrics available

The USER peer sends the following message using MAILBOX SEND to
FTY-METRIC-CACHE-SERVER ("fty-metric-cache") peer:

* zuuid/LIST - request list of devices with (some) metrics available

where
* '/' indicates a multipart string message
* subject of the message MUST be "latest-rt-data".

The FTY-METRIC-CACHE-SERVER peer MUST respond with this message back to USER
peer using MAILBOX SEND.

* zuuid/OK/LIST/devices

where
* '/' indicates a multipart frame message
* 'devices' is a string of device names separated by newline
* subject of the message MUST be "latest-rt-data".

#### Get current metrics for specified asset

The USER peer sends the following message using MAILBOX SEND to
FTY-METRIC-CACHE-SERVER ("fty-metric-cache") peer:

* zuuid/GET/element/[filter] - request current metrics for asset 'element'

where
* '/' indicates a multipart string message
* 'element' MUST be name of existing asset or a regex about asset name 
* 'filter' is an optional parameter which filter as a regex type of metric
* subject of the message MUST be "latest-rt-data".

The FTY-METRIC-CACHE-SERVER peer MUST respond with this  message back to USER
peer using MAILBOX SEND.

* zuuid/OK/GET/element/metric-1/.../metric-n

where
* '/' indicates a multipart frame message
* 'metric-1',...,'metric-n' are ALL the current metrics (with valid TTL) available for 'element'
* subject of the message MUST be "latest-rt-data".

### Stream subscriptions

Agent is subscribed to METRICS stream.

Received metrics are stored into local cache.

