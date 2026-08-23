# DL1 Spawner

A simple in-game item spawner and duplication tool for **Dying Light 1**.

![Item Spawner](Spawner.png)

![Item Duper](Spawner1.png)

## Features

### Item Spawner

Spawn any item using its internal item ID and the desired quantity.

Press the **End** key (`VK_END`) at any time to dump the currently available item IDs. The results will be written to:

```text
Dying Light\dumped_items.txt
```

A copy of `dumped_items.txt` is included in this repository for convenience. If an ID from the included list no longer works, dump the IDs again from your current game version.

### Item Duper

Allows inventory items to be dropped without removing them from the player's inventory.

Enable the duper, drop an item normally, and it will remain in your inventory. This can be repeated as many times as needed.

## Installation

### Precompiled Release

1. Download the latest build from the [Releases](../../releases) page.
2. Launch Dying Light.
3. Enter a loaded save.
4. Inject the DLL into the Dying Light process using a DLL injector.
5. Open the menu and select the feature you want to use.

[Xenos](https://github.com/darthton/xenos) is one injector that can be used.

### Building From Source

1. Clone or download this repository.
2. Open the solution in Visual Studio.
3. Select **Release** and **x64**.
4. Build the solution.
5. Inject the compiled DLL into the Dying Light process.

## Usage

To spawn an item:

1. Open the item spawner.
2. Enter a valid item ID.
3. Enter the desired quantity.
4. Press the spawn button.

To refresh the item list, press **End** while in-game and check `dumped_items.txt` in the game directory.

To duplicate an item:

1. Enable the item duper.
2. Open your inventory.
3. Drop the item normally.
4. Pick up the dropped copy or continue dropping additional copies.

## Notes

* Intended for the Windows version of Dying Light 1.
* Game updates may change or invalidate item IDs.
* Antivirus software may flag injected DLLs

## Disclaimer

This project is not affiliated with or endorsed by Techland. Use it at your own risk.

If you find the project useful, consider starring the repository.
