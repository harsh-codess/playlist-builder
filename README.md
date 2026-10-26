# Playlist Builder

A C++17 media library and playlist builder with a browser UI. Add songs, podcast
episodes and audiobooks (or scan a music folder), play them in the browser, build
playlists, and export them as `.m3u` files that VLC, Music or any player can open.

It is the OOP-in-C++ mini project: the server, JSON, HTTP and every rule about the
data are written in plain C++ with no third-party libraries.

## Run it

```sh
make            # builds bin/playlist-builder
make run        # starts http://127.0.0.1:8080
make test       # runs the 27 unit tests
make report     # rebuilds report/Mini_Project_Report.pdf
```

Run from this folder. On the first start the server creates `data/library.json`
and synthesises a few short WAV clips in `data/samples/` so there is something
to play right away.

Options: `--port 8080`, `--host 127.0.0.1`, `--data data/library.json`,
`--public public`, `--samples data/samples`. Setting the `PORT` environment
variable binds to `0.0.0.0` on that port (used by the Docker image).

```sh
docker build -t playlist-builder .
docker run -p 8080:8080 playlist-builder
```

### Deploy to Render

`render.yaml` is a Render Blueprint. In the Render dashboard choose
**New → Blueprint**, pick this repository, and Render builds the Dockerfile and
serves it on a public `onrender.com` URL. On the free plan the disk is
ephemeral, so the library resets to the demo data after a redeploy or when the
service sleeps.

## Using it

- **Library** (home icon): search, filter by type or favourites, sort, click to play.
- **Playlists** (list icon): create, rename, duplicate, shuffle, reorder, export `.m3u`.
- **Add** (waveform icon): add one item by file path or `http(s)` URL, or scan a whole
  folder. Files named `Artist - Title.mp3` are split into artist and title.
- **Activity** (bell icon): everything that happened this session, including errors
  with the exception type that caused them.

## How the OOP concepts are used

| Concept | Where |
|---|---|
| Constructors | `Media`, `Song`, `PodcastEpisode`, `Audiobook` and `Playlist` validate in their constructors and throw instead of creating a bad object. `Playlist` has an explicit copy constructor used by "Duplicate". `FolderScanner` uses a delegating constructor. |
| Inheritance | `Song`, `PodcastEpisode`, `Audiobook` derive from the abstract `Media`. Every error derives from `AppException`, which derives from `std::runtime_error`. |
| Polymorphism | `Media` has pure virtual `type()`, `typeLabel()`, `subtitle()`, `writeFields()` and overridable `m3uLabel()` / `matches()`. Playlists and the library hold `shared_ptr<Media>` and call these through the base pointer. `AppException::type()` is virtual so one `catch` reports the exact error. Operators `+=`, `[]`, `<` and `<<` are overloaded. |
| STL | `unordered_map` (library index), `vector` (ordering), `map`, `set`, `pair`, `optional`, `function`, `shared_ptr`/`unique_ptr`, and algorithms `copy_if`, `stable_sort`, `rotate`, `shuffle`, `accumulate`, `remove_if`, `replace_if`, `any_of`, `transform`, `mismatch`. `std::filesystem` for scanning, `std::thread` + `std::mutex` for the server. |
| Templates | `Registry<T>` stores playlists by id; `expectThrow<E>` in the tests. |
| Exception handling | `InvalidMediaException` (400), `BadRequestException` (400), `NotFoundException` (404), `MethodNotAllowedException` (405), `DuplicateException` (409), `EmptyPlaylistException` (422), `StorageException` (500). `PlaylistApp::handle` turns any of them into `{"error": {type, message, status}}`, and the UI shows it. |

## Layout

```
include/   headers: media, library, playlist, registry, storage, scanner, seeder, json, http, app, exceptions
src/       implementations + main.cpp
public/    index.html, styles.css, app.js (the browser UI)
tests/     test_core.cpp (unit tests)
data/      library.json and generated samples (created on first run, git-ignored)
report/    Mini_Project_Report.pdf (MIT-WPU format) and assets
```

## API

| Method | Path | Purpose |
|---|---|---|
| GET | `/api/media?q=&type=&sort=` | search (`type`: all, song, podcast, audiobook, favorites; `sort`: recent, title, creator, duration, plays) |
| POST / PUT / DELETE | `/api/media[/:id]` | add, update, delete |
| POST | `/api/media/:id/favorite`, `/play`, `/duration` | toggle favourite, count a play, store a measured duration |
| POST | `/api/scan` `{folder}` | add every audio file in a folder |
| GET | `/api/stats` | totals |
| GET / POST | `/api/playlists` | list, create |
| GET / PUT / DELETE | `/api/playlists/:id` | detail, rename, delete |
| POST | `/api/playlists/:id/tracks`, `/move`, `/shuffle`, `/duplicate` | edit |
| DELETE | `/api/playlists/:id/tracks/:index` | remove one track |
| GET | `/api/playlists/:id/export.m3u` | download |
| GET | `/stream/:id` | audio bytes with HTTP Range support |
