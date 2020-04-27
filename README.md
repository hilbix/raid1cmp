# RAID1cmd

Get a list of the position of the mismatches:

	# cat /sys/block/md*/md/mismatch_cnt

## Usage

	git clone https://github.com/hilbix/raid1cmp.git
	cd raid1cmp
	make
	sudo make install

and then

	sudo raid1cmp /dev/sda1 /dev/sdb1

or call the aux script to autodetect devices:

	sudo raid1cmp.sh /sys/block/md*

## Output

Progress is written each second to STDERR:

- YYYYmmdd
- HHMMSS
- Round
- Mismatch count
- Blocksize (4096)
- Position (so position is `Position * Blocksize`)
- `CR`
- `LF` if the program terminates

Other output to STDERR looks like:

- YYYYmmdd
- HHMMSS
- `0`
- Message
- `LF` if the program terminates

Mismatches are written to STDOUT:

- YYYYmmdd
- HHMMSS
- Round
- Mismatch count
- Blocksize (4096)
- Start (so position is `Start * Blocksize`)
- Count (so end is `(Start + Count) * Blocksize -1`)
- Mismatch number
- Byte count of mismatches
- `LF`

Read errors are written to STDOUT: (Read errors are dead slow)

- YYYYmmdd
- HHMMSS
- `0`
- Read error count
- Blocksize (4096)
- Start (so position is `Start * Blocksize`)
- Count (so far)
- Read error number
- Byte count of dead bytes
- Device
- `LF`
- If `Count` becomes `0` this means, the mismatch has gone
  - Read error blocks do not count as mismatch

## Input

Input from a `TTY` is set into `CBREAK`-Mode.

- While in a round it nonblockingly read single characters from STDIN.
- After each round it blockingly reads a single character from STDIN.
  - This blocking will be interrupted by timer events, see below
  - If there is a next round count, it does not block

If a character is got:

- If nothing comes in, it continues with the (possibly new) round
- On an empty line (just `LF` or `CR`), it increments the next round count
- On backspace it "eats" the first LF (this only works on a `TTY`)
- On EOF it finishs when the round is finished
  - EOF must be permanent.  This means it must be read multiple times.
- On `.` it does the read loop again, hence delays a bit longer
- Else it reads the line
  - If the line starts with a number it is understood as read error line
  - Else the line is fed to `system()`.

So:

- `</dev/null` to terminate after the first round when mismatches are found
- `<<< ..........` to delay the 2nd loop for 1s and terminate afterwards
- `yes .......... |` to delay each loop by 1s
- `yes '' | head -10 |` to terminate after 11 rounds without waiting between rounds


## Environment

`raid1cmp` has no options, but can be controlled by environment variables:

- `BLOCKSIZE=4096`.  Must be a power of 2.
- `BLOCKCOUNT=0` use `CHUNKSIZE` to read.  Number of blocks to read by each system call.
  - `BLOCKSIZE * BLOCKCOUNT` is always kept in the range of `1K..1G` (1024..1073741824).
- `CHUNKSIZE=128M` the default chunksize
  - Must be in the range `1K` to `1G` (1024 to 1073741824)
- `DIRECTIO=` (empty or unset) use direct IO on all devices
  - Use `DIRECTIO=0` to disable direct IO on all devices
  - Use `DIRECTIO='/dev/1 /dev/2'` to only enable it on the given device
  - You must use the exact path as given on commandline
  - If the devices have a space in their name, the result is not defined

Example:

	BLOCKSIZE=512 BLOCKCOUNT=2048 DIRECTIO=/dev/sda2 nice -99 ionice -c3 slowdown 1 raid1cmp /dev/sd[ab]2

When `raid1cmp` forks a command, it sets following environment variables:

- `THEBLOCKSIZE` to the `BLOCKSIZE` in effect
- `THEBLOCKCOUNT` to the `BLOCKCOUNT` in effect
- `THECHUNKSIZE` to the `CHUNKSIZE` in effect
- `THEDIRECTIO` to the devices which use direct IO
- `THEPID` to it's own `PID`
- `THEDEVS` to the devices given on commandline
- `THEBLOCK` to the current block position (where the next read comes from)
- `THEPOS` to the current byte position (`THECOUNT * THEBLOCKSIZE`)
- `THEERRORCNT` to the number of defective blocks seen so far
- `THEROUND` the current round number
- `THEMISMATCHCNT` the current number of mismatches so far


## FAQ

License?

- Free as in free beer, free speech and free baby.

Mounted?

- This is readonly

Busy FS?

- It continuously re-checks all changes
- It only returns when there are no changes
- use `raid1cmp.sh </dev/null` to terminate after the first round
- However the first round may be inaccurate
- Use `echo |` for the next round, however this might be inaccurate, too
- In general, each LF allows another round and EOF terminates

Control?

- Use `nice -99 ionice -c3` as shown in the example
- Enter `sh` or `bash` (and return) to fork a shell
  - You can use `renice 99 $THEPID` or `ionice -c3 -p$THEPID` afterwards

Delay command to the end of a round?

- This is always needed, regardless of `TTY` or pipes
- Use `LF` (empty line) in front of the command
  - The `LF` waits for the end of the round
  - Do not press BACKSPACE, as this cancels the effect of the (first) `LF`.
- Else everything is always executed immediately
- Hence the right thing to do from scripts is:
  - First pipe in the list of defects to skip (possbly from a previous run)
  - Send an empty line
  - Then send the commands to execute after the round

