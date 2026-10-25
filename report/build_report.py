#!/usr/bin/env python3
"""Build the OOP mini-project report in the MIT-WPU sample format."""

from io import BytesIO
from pathlib import Path

from PIL import Image as PILImage
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_JUSTIFY, TA_LEFT, TA_RIGHT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import inch, mm
from reportlab.platypus import (
    BaseDocTemplate,
    Frame,
    Image,
    KeepTogether,
    ListFlowable,
    ListItem,
    NextPageTemplate,
    PageBreak,
    PageTemplate,
    Paragraph,
    Preformatted,
    Spacer,
    Table,
    TableStyle,
)

ROOT = Path(__file__).resolve().parent
ASSETS = ROOT / "assets"
OUT = ROOT / "Mini_Project_Report.pdf"

PAGE_W, PAGE_H = A4
NAVY = colors.HexColor("#1B365D")
LINE = colors.HexColor("#333333")
PALE = colors.HexColor("#F4F6F8")
RED = colors.HexColor("#C00000")
CODE_BG = colors.HexColor("#F3F3F3")

TITLE = "Playlist Builder"
FULL_TITLE = "Playlist Builder: An Object Oriented Media Library"
MEMBERS = [
    "Harsh Gidwani",
    "Ansh Thakare",
    "Darshan Nayak",
    "Anushka Wani",
]
STUDENT = ", ".join(MEMBERS)
CLASS_LINE = "SY B.Tech. (Artificial Intelligence & Data Science)"
COURSE = "Object Oriented Programming in CPP"
COURSE_CODE = "CSE1003B"
FACULTY = "Prof. Poonam Bhosle"
YEAR = "2026-2027"
SEM = "Sem-III"

page_map = {}


class Anchor(Spacer):
    def __init__(self, name):
        super().__init__(0, 0)
        self._name = name

    def draw(self):
        page_map[self._name] = self.canv.getPageNumber()
        self.canv.bookmarkPage(self._name)


def styles():
    base = getSampleStyleSheet()
    s = {
        "cover_small": ParagraphStyle(
            "cover_small", fontName="Times-Roman", fontSize=13, leading=18,
            alignment=TA_CENTER, textColor=colors.black, spaceAfter=4,
        ),
            "cover_title": ParagraphStyle(
            "cover_title", fontName="Times-Bold", fontSize=16, leading=21,
            alignment=TA_CENTER, textColor=RED, spaceBefore=6, spaceAfter=6,
        ),
        "cover_name": ParagraphStyle(
            "cover_name", fontName="Times-Bold", fontSize=13, leading=18,
            alignment=TA_CENTER, textColor=RED, spaceAfter=2,
        ),
        "cover_class": ParagraphStyle(
            "cover_class", fontName="Times-Bold", fontSize=12, leading=16,
            alignment=TA_CENTER, spaceBefore=2, spaceAfter=10,
        ),
        "cover_dept": ParagraphStyle(
            "cover_dept", fontName="Times-Bold", fontSize=13, leading=18,
            alignment=TA_CENTER, spaceBefore=0,
        ),
        "cert_dept": ParagraphStyle(
            "cert_dept", fontName="Times-Bold", fontSize=13, leading=18,
            alignment=TA_CENTER, spaceBefore=8, spaceAfter=18,
        ),
        "cert_head": ParagraphStyle(
            "cert_head", fontName="Times-Bold", fontSize=16, leading=20,
            alignment=TA_CENTER, spaceBefore=12, spaceAfter=28,
            letterSpacing=3,
        ),
        "body": ParagraphStyle(
            "body", fontName="Times-Roman", fontSize=12, leading=18,
            alignment=TA_JUSTIFY, spaceAfter=10, firstLineIndent=0,
        ),
        "body_indent": ParagraphStyle(
            "body_indent", fontName="Times-Roman", fontSize=12, leading=18,
            alignment=TA_JUSTIFY, spaceAfter=10, firstLineIndent=18,
        ),
        "center": ParagraphStyle(
            "center", fontName="Times-Roman", fontSize=12, leading=18,
            alignment=TA_CENTER, spaceAfter=8,
        ),
        "center_red": ParagraphStyle(
            "center_red", fontName="Times-Bold", fontSize=12, leading=18,
            alignment=TA_CENTER, textColor=RED, spaceAfter=8,
        ),
        "h_front": ParagraphStyle(
            "h_front", fontName="Times-Bold", fontSize=22, leading=28,
            alignment=TA_CENTER, spaceBefore=8, spaceAfter=28,
        ),
        "chapter": ParagraphStyle(
            "chapter", fontName="Times-Bold", fontSize=22, leading=28,
            alignment=TA_CENTER, spaceBefore=6, spaceAfter=4,
        ),
        "chapter_sub": ParagraphStyle(
            "chapter_sub", fontName="Times-Bold", fontSize=18, leading=24,
            alignment=TA_CENTER, spaceAfter=22,
        ),
        "h2": ParagraphStyle(
            "h2", fontName="Times-Bold", fontSize=13.5, leading=18,
            alignment=TA_LEFT, spaceBefore=12, spaceAfter=8, textColor=NAVY,
        ),
        "caption": ParagraphStyle(
            "caption", fontName="Times-Italic", fontSize=11, leading=15,
            alignment=TA_CENTER, spaceBefore=6, spaceAfter=14,
        ),
        "toc": ParagraphStyle(
            "toc", fontName="Times-Roman", fontSize=12, leading=20,
            alignment=TA_LEFT,
        ),
        "code": ParagraphStyle(
            "code", fontName="Courier", fontSize=8, leading=10.5,
            alignment=TA_LEFT, backColor=CODE_BG, leftIndent=6, rightIndent=6,
            spaceBefore=6, spaceAfter=10,
        ),
        "th": ParagraphStyle(
            "th", fontName="Times-Bold", fontSize=10, leading=13,
            alignment=TA_CENTER,
        ),
        "td": ParagraphStyle(
            "td", fontName="Times-Roman", fontSize=9.5, leading=13,
            alignment=TA_LEFT,
        ),
        "tdc": ParagraphStyle(
            "tdc", fontName="Times-Roman", fontSize=9.5, leading=13,
            alignment=TA_CENTER,
        ),
        "sign": ParagraphStyle(
            "sign", fontName="Times-Roman", fontSize=12, leading=16,
            alignment=TA_LEFT,
        ),
        "footer": ParagraphStyle(
            "footer", fontName="Times-Roman", fontSize=11, leading=14,
            alignment=TA_CENTER,
        ),
    }
    return s


S = styles()


def p(text, style="body"):
    return Paragraph(text, S[style])


def bullets(items):
    flow = []
    for item in items:
        flow.append(ListItem(p(item), leftIndent=18, bulletColor=colors.black))
    return ListFlowable(flow, bulletType="bullet", start="•", leftIndent=22, bulletFontName="Times-Roman")


def numbered(items):
    flow = []
    for item in items:
        flow.append(ListItem(p(item), leftIndent=18))
    return ListFlowable(flow, bulletType="1", leftIndent=22, bulletFontName="Times-Roman")


def table(data, col_widths, header=True):
    styled = []
    for i, row in enumerate(data):
        out = []
        for j, cell in enumerate(row):
            if isinstance(cell, str):
                style = "th" if header and i == 0 else ("tdc" if j == 0 else "td")
                out.append(Paragraph(cell, S[style]))
            else:
                out.append(cell)
        styled.append(out)
    t = Table(styled, colWidths=col_widths, repeatRows=1 if header else 0)
    cmds = [
        ("FONTNAME", (0, 0), (-1, 0), "Times-Bold") if header else ("FONTNAME", (0, 0), (-1, -1), "Times-Roman"),
        ("GRID", (0, 0), (-1, -1), 0.6, LINE),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 6),
        ("RIGHTPADDING", (0, 0), (-1, -1), 6),
        ("TOPPADDING", (0, 0), (-1, -1), 5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
        ("BACKGROUND", (0, 0), (-1, 0), PALE) if header else ("BACKGROUND", (0, 0), (-1, -1), colors.white),
    ]
    t.setStyle(TableStyle([c for c in cmds if c]))
    return t


def figure(path, caption, width=6.4 * inch, key=None):
    img_path = ASSETS / path
    # Compress large UI screenshots so the PDF stays a reasonable size.
    pil = PILImage.open(img_path).convert("RGB")
    max_px = 1600
    if max(pil.size) > max_px:
        pil.thumbnail((max_px, max_px), PILImage.Resampling.LANCZOS)
    buf = BytesIO()
    pil.save(buf, format="JPEG", quality=82, optimize=True)
    buf.seek(0)
    im = Image(buf)
    scale = width / im.imageWidth
    im.drawWidth = width
    im.drawHeight = im.imageHeight * scale
    max_h = 5.4 * inch
    if im.drawHeight > max_h:
        s2 = max_h / im.drawHeight
        im.drawWidth *= s2
        im.drawHeight *= s2
    bits = []
    if key:
        bits.append(Anchor(key))
    bits.extend([im, p(caption, "caption")])
    return KeepTogether(bits)


def code_block(text):
    return Preformatted(text.strip("\n"), S["code"])


def footer_roman(canvas, doc):
    canvas.saveState()
    canvas.setFont("Times-Roman", 12)
    # Cover is unnumbered; certificate is I.
    canvas.drawCentredString(PAGE_W / 2, 18 * mm, _to_roman(doc.page - 1))
    canvas.restoreState()


def footer_arabic(canvas, doc):
    canvas.saveState()
    canvas.setFont("Times-Roman", 12)
    canvas.drawCentredString(PAGE_W / 2, 18 * mm, str(doc.page - _FRONT_COUNT))
    canvas.restoreState()


def footer_none(canvas, doc):
    pass


def _to_roman(n):
    vals = [
        (1000, "M"), (900, "CM"), (500, "D"), (400, "CD"),
        (100, "C"), (90, "XC"), (50, "L"), (40, "XL"),
        (10, "X"), (9, "IX"), (5, "V"), (4, "IV"), (1, "I"),
    ]
    out = []
    for v, s in vals:
        while n >= v:
            out.append(s)
            n -= v
    return "".join(out)


_FRONT_COUNT = 5  # cover + certificate + abstract + figures + contents


def cover():
    story = []
    story.append(Spacer(1, 6 * mm))
    logo = Image(str(ASSETS / "logo_cover.png"), width=36 * mm, height=34 * mm)
    logo.hAlign = "CENTER"
    story.append(logo)
    story.append(Spacer(1, 6 * mm))
    story.append(p("Mini Project Report", "cover_small"))
    story.append(p("on", "cover_small"))
    story.append(p(FULL_TITLE, "cover_title"))
    story.append(Spacer(1, 4 * mm))
    story.append(p("Submitted by", "cover_small"))
    story.append(Spacer(1, 6 * mm))
    story.append(p("Project Members", "cover_small"))
    story.append(Spacer(1, 2 * mm))
    for name in MEMBERS:
        story.append(p(name, "cover_name"))
    story.append(p(CLASS_LINE, "cover_class"))
    story.append(Spacer(1, 3 * mm))
    story.append(p(f"Course : {COURSE}", "cover_small"))
    story.append(Spacer(1, 2 * mm))
    story.append(p(f"Course Code : {COURSE_CODE}", "cover_small"))
    story.append(Spacer(1, 8 * mm))
    story.append(p("Under the Internal Guidance of", "cover_small"))
    story.append(Spacer(1, 3 * mm))
    story.append(p(FACULTY, "cover_name"))
    story.append(Spacer(1, 8 * mm))
    story.append(p("Department of Computer Engineering and Technology<br/>"
                   "MIT World Peace University, Kothrud,<br/>"
                   "Pune 411 038, Maharashtra - India<br/>"
                   f"{YEAR}", "cover_dept"))
    story.append(NextPageTemplate("front"))
    story.append(PageBreak())
    return story


def certificate():
    story = [Anchor("certificate")]
    header = Image(str(ASSETS / "logo_header.png"), width=170 * mm, height=49 * mm)
    header.hAlign = "CENTER"
    story.append(header)
    story.append(p("DEPARTMENT OF COMPUTER ENGINEERING AND TECHNOLOGY", "cert_dept"))
    story.append(p("C E R T I F I C A T E", "cert_head"))
    story.append(p("This is to certify that,", "center"))
    for name in MEMBERS:
        story.append(p(name, "center_red"))
    story.append(Spacer(1, 4 * mm))
    story.append(p(
        f"of SY-BTech. (Artificial Intelligence &amp; Data Science) have completed their mini project titled "
        f"“<font color='#C00000'>{FULL_TITLE}</font>” and have submitted this Object Oriented Programming in CPP "
        f"Report for the academic year {YEAR} ({SEM}).",
        "body",
    ))
    story.append(Spacer(1, 22 * mm))
    sign = [
        [Paragraph(FACULTY, S["sign"])],
        [Paragraph("Mini Project Guide", S["sign"])],
        [Paragraph("Department of CET", S["sign"])],
        [Paragraph("MIT World Peace University, Pune", S["sign"])],
    ]
    t = Table(sign, colWidths=[90 * mm])
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 0)]))
    story.append(t)
    story.append(PageBreak())
    return story


def abstract_page():
    story = [Anchor("abstract"), p("Abstract", "h_front")]
    story.append(p(
        "Playlist Builder is a C++17 mini project that stores songs, podcast episodes and audiobook "
        "chapters in one library, lets the user assemble playlists, and exports those playlists as "
        "standard M3U (.m3u) playlist files. The program is a small HTTP server with a "
        "browser user interface, so the same binary is both the object-oriented core and a usable "
        "desktop tool."
    ))
    story.append(p(
        "The domain model is built around an abstract base class <font face='Courier'>Media</font>. "
        "Three derived classes — <font face='Courier'>Song</font>, <font face='Courier'>PodcastEpisode</font> "
        "and <font face='Courier'>Audiobook</font> — add their own fields and override virtual functions "
        "for search, display and playlist export. Constructors validate every object before it exists. "
        "A hierarchy of exceptions derived from <font face='Courier'>AppException</font> reports empty "
        "titles, missing files, duplicate sources and empty playlists as JSON errors instead of crashing. "
        "The Standard Template Library supplies the library index (<font face='Courier'>unordered_map</font>), "
        "playlist order (<font face='Courier'>vector</font>), search and sort algorithms, and a "
        "template registry for playlists."
    ))
    story.append(p(
        "A seed library of twelve short WAV clips is generated on first launch so the player can be "
        "demonstrated immediately. Twenty-seven unit tests cover constructors, polymorphism, persistence "
        "and export. The browser interface follows a dark, glass-style music player layout with a now-playing "
        "disc, a searchable library, playlist editing and folder scanning."
    ))
    story.append(PageBreak())
    return story


def figures_page():
    story = [Anchor("figures"), p("List of Figures &amp; Tables", "h_front")]
    data = [
        ["SrNo", "Title", "Page No"],
        ["1", "Class diagram of Playlist Builder", toc_page("fig-class")],
        ["2", "Now playing and library user interface", toc_page("fig-ui")],
        ["3", "Playlist list and mini player", toc_page("fig-pl")],
        ["4", "Playlist detail with export actions", toc_page("fig-pld")],
        ["5", "Add-media form (Song / Podcast / Audiobook)", toc_page("fig-add")],
        ["6", "Constructor exception shown in the browser", toc_page("fig-err")],
        ["T1", "List of classes and their roles", toc_page("tab-classes")],
        ["T2", "Exception hierarchy and HTTP status codes", toc_page("tab-ex")],
        ["T3", "Unit test cases and results", toc_page("tab-tests")],
        ["T4", "Individual contributions", toc_page("tab-contrib")],
    ]
    story.append(table(data, [22 * mm, 110 * mm, 28 * mm]))
    story.append(PageBreak())
    return story


def toc_page(key):
    n = page_map.get(key)
    if n is None:
        return "  "
    if n <= _FRONT_COUNT:
        return _to_roman(n - 1)
    return str(n - _FRONT_COUNT)


def contents_page():
    story = [Anchor("contents"), p("Contents", "h_front")]
    story.append(p(f"Abstract {_dots()} {toc_page('abstract')}", "toc"))
    story.append(p(f"List of Figures &amp; Tables {_dots()} {toc_page('figures')}", "toc"))
    story.append(Spacer(1, 8 * mm))
    data = [
        ["Chapter No", "Particulars", "PageNo"],
        ["1", "Introduction", toc_page("ch1")],
        ["2", "Problem Statement", toc_page("ch2")],
        ["3", "Objectives", toc_page("ch3")],
        ["4", "List of classes and class diagram", toc_page("ch4")],
        ["5", "Description of OOP concepts used", toc_page("ch5")],
        ["6", "Implementation Details and code", toc_page("ch6")],
        ["7", "Test Cases and Results", toc_page("ch7")],
        ["8", "Individual Contributions", toc_page("ch8")],
        ["9", "Conclusion and Future Scope", toc_page("ch9")],
    ]
    story.append(table(data, [32 * mm, 100 * mm, 28 * mm]))
    story.append(NextPageTemplate("body"))
    story.append(PageBreak())
    return story


def _dots():
    return "." * 8


def chapter_head(num, title, key):
    return [Anchor(key), p(f"Chapter {num}", "chapter"), p(title, "chapter_sub")]


def ch1():
    s = chapter_head(1, "Introduction", "ch1")
    s.append(p(
        "Object Oriented Programming treats a program as a set of objects that hide their data and "
        "expose behaviour through well-defined operations. For a media library this is a natural fit: "
        "a song, a podcast episode and an audiobook chapter all have a title, a creator, a duration and "
        "a source file, yet each needs a different subtitle, a different search rule and a different "
        "line in a playlist file."
    ))
    s.append(p(
        "Playlist Builder was written for the Object Oriented Programming in CPP course (CSE1003B). "
        "The assignment required constructors, inheritance, polymorphism, the Standard Template Library "
        "and exception handling to appear as working parts of one program, not as isolated classroom "
        "examples. The program is therefore a complete tool: a C++ HTTP server that stores a library "
        "on disk, serves a browser player, and writes .m3u playlists that "
        "VLC or Apple Music can open."
    ))
    s.append(p("1.1 Scope", "h2"))
    s.append(p(
        "The first release covers local files and HTTP(S) URLs, three media types, any number of "
        "playlists, folder scanning, search and sort, favourite flags, and JSON persistence. It does "
        "not stream commercial catalogues, does not require user accounts, and does not depend on "
        "third-party C++ libraries. JSON, HTTP and WAV generation are implemented in the project itself "
        "so every line that enforces a rule can be shown in this report."
    ))
    s.append(p("1.2 Organisation of the report", "h2"))
    s.append(p(
        "Chapter 2 states the problem. Chapter 3 lists the objectives. Chapter 4 names every class and "
        "shows the class diagram. Chapter 5 maps each required OOP concept onto concrete code. "
        "Chapter 6 describes the implementation, the HTTP API and selected listings. Chapter 7 records "
        "the unit tests and the results of running them. Chapter 8 records individual contributions. "
        "Chapter 9 concludes and outlines future work."
    ))
    s.append(PageBreak())
    return s


def ch2():
    s = chapter_head(2, "Problem Statement", "ch2")
    s.append(p(
        "People keep audio in mixed forms: MP3 albums, podcast feeds saved as files, and audiobook "
        "chapters. Consumer players treat these as a flat list of files. Course exercises that only "
        "print “Song created” never have to decide what happens when a title is blank, a file is "
        "missing, or two items share the same path."
    ))
    s.append(p(
        "The problem addressed by this mini project is: design and implement a type-safe media library "
        "in C++ in which songs, podcast episodes and audiobook chapters are related by inheritance, "
        "are stored and searched with the STL, cannot be constructed in an invalid state, and can be "
        "grouped into playlists that export a real .m3u file, with every "
        "failure reported through a typed exception that the user interface can display."
    ))
    s.append(p("2.1 Why a web interface", "h2"))
    s.append(p(
        "A console menu would have been enough to call the classes. A browser player was chosen so "
        "the same objects can be seen, heard and exported. The interface is a pair of phone-sized "
        "panels (now playing and library) inspired by contemporary music-player design, with a purple "
        "gradient, a glass navigation bar and a circular progress disc. Polymorphism is visible in "
        "the type chip and subtitle of every row; exception handling is visible when the form reports "
        "<font face='Courier'>InvalidMediaException</font> instead of failing silently."
    ))
    s.append(PageBreak())
    return s


def ch3():
    s = chapter_head(3, "Objectives", "ch3")
    s.append(p("The mini project was planned against the following objectives."))
    s.append(numbered([
        "To design an abstract <font face='Courier'>Media</font> base class and three derived types "
        "(<font face='Courier'>Song</font>, <font face='Courier'>PodcastEpisode</font>, "
        "<font face='Courier'>Audiobook</font>) that share identity, duration and source while keeping "
        "type-specific fields.",
        "To validate every object in its constructor so an empty title, a negative duration, an unknown "
        "year or a missing episode number never becomes a live object.",
        "To use virtual functions so a playlist of <font face='Courier'>shared_ptr&lt;Media&gt;</font> "
        "can export, search and display each item without a chain of type tests.",
        "To store the library in STL containers (<font face='Courier'>unordered_map</font> for lookup, "
        "<font face='Courier'>vector</font> for order) and to search, sort, move and shuffle with "
        "standard algorithms.",
        "To report user and storage failures through an exception hierarchy derived from "
        "<font face='Courier'>AppException</font>, mapped to HTTP status codes and shown in the UI.",
        "To persist the library as JSON, seed a demo catalogue on first run, and export playlists as "
        ".m3u files that other players accept.",
        "To present the library in a browser player that can play local WAV/MP3 files, build playlists "
        "and scan a folder of audio files.",
        "To verify the design with automated unit tests covering constructors, polymorphism, persistence "
        "and export.",
    ]))
    s.append(PageBreak())
    return s


def ch4():
    s = chapter_head(4, "List of classes and class diagram", "ch4")
    s.append(p("4.1 List of classes", "h2"))
    s.append(Anchor("tab-classes"))
    classes = [
        ["Class", "Kind", "Responsibility"],
        ["Media", "Abstract base", "Shared identity, validation, JSON, M3U entry, live-object count."],
        ["Song", "Derived", "Album, genre, year; artist-title M3U label; album/genre search."],
        ["PodcastEpisode", "Derived", "Show name and episode number; episode-aware subtitle and search."],
        ["Audiobook", "Derived", "Narrator and chapter; chapter-aware subtitle and search."],
        ["MediaFactory", "Factory", "Builds the correct derived class from a JSON body."],
        ["Library", "Container", "Owns every Media item; O(1) lookup; filter, search and sort."],
        ["Playlist", "Aggregate", "Ordered shared tracks; move, shuffle, duration, M3U export."],
        ["Registry&lt;T&gt;", "Class template", "Id-keyed store used for playlists; next-id generation."],
        ["AppException", "Exception base", "Message plus HTTP status; virtual type() name."],
        ["InvalidMediaException", "Exception", "Constructor / field validation failures (HTTP 400)."],
        ["BadRequestException", "Exception", "Malformed JSON or out-of-range playlist index (400)."],
        ["NotFoundException", "Exception", "Unknown media, playlist or folder (404)."],
        ["DuplicateException", "Exception", "Repeated id or source path (409)."],
        ["EmptyPlaylistException", "Exception", "Export of a playlist with no tracks (422)."],
        ["StorageException", "Exception", "Unreadable or unwritable save file (500)."],
        ["Json", "Value type", "Minimal JSON parser and writer used for the API and save file."],
        ["Storage", "Service", "Load and save library.json atomically."],
        ["FolderScanner", "Service", "Walk a directory, parse “Artist - Title”, read WAV duration."],
        ["Seeder / SampleGenerator", "Service", "Create the demo library and synthesise short WAV clips."],
        ["PlaylistApp", "Application", "HTTP routes, mutex, persistence after every mutation."],
        ["Router / HttpServer", "Infrastructure", "Path matching, static files, Range requests, threads."],
    ]
    s.append(table(classes, [42 * mm, 32 * mm, 86 * mm]))
    s.append(p(
        "Table 1. Classes in Playlist Builder and the role each one plays.",
        "caption",
    ))
    s.append(p("4.2 Class diagram", "h2"))
    s.append(p(
        "Figure 1 shows inheritance from <font face='Courier'>Media</font> and "
        "<font face='Courier'>AppException</font>, aggregation of tracks inside a playlist, composition "
        "of the library and registry inside <font face='Courier'>PlaylistApp</font>, and the template "
        "parameter of <font face='Courier'>Registry&lt;T&gt;</font>."
    ))
    s.append(figure("class-diagram.png", "Figure 1. Class diagram of Playlist Builder.", 6.6 * inch, "fig-class"))
    s.append(PageBreak())
    return s


def ch5():
    s = chapter_head(5, "Description of OOP concepts used", "ch5")
    s.append(p("5.1 Constructors", "h2"))
    s.append(p(
        "A constructor is the only path that creates a valid object. "
        "<font face='Courier'>Media::Media</font> trims and rejects empty titles, creators and sources, "
        "and rejects negative durations. <font face='Courier'>Song</font> additionally checks that a "
        "non-zero year lies in 1900–2100. <font face='Courier'>PodcastEpisode</font> and "
        "<font face='Courier'>Audiobook</font> require episode and chapter numbers of at least 1. "
        "<font face='Courier'>Playlist</font> rejects a blank name. Because these checks throw "
        "<font face='Courier'>InvalidMediaException</font> (or <font face='Courier'>BadRequestException</font>) "
        "before the object is usable, the rest of the program never has to test “is this title empty?”."
    ))
    s.append(p(
        "<font face='Courier'>Playlist</font> also has an explicit copy constructor, used by the "
        "Duplicate action: the copy shares the same <font face='Courier'>Media</font> objects "
        "(aggregation) but has its own name and track vector. "
        "<font face='Courier'>FolderScanner</font> uses a delegating constructor for its default "
        "audio-extension set. A static live-object counter is incremented in every "
        "<font face='Courier'>Media</font> constructor and decremented in the virtual destructor, "
        "which is how the tests prove that derived objects are destroyed through a base pointer."
    ))
    s.append(code_block("""
Media::Media(std::string id, std::string title, std::string creator,
             int durationSeconds, std::string source)
    : id_(std::move(id)),
      title_(requireText(title, "Title")),
      creator_(requireText(creator, "Creator")),
      source_(requireText(source, "Source", 2048)) {
    if (id_.empty()) throw InvalidMediaException("Media id cannot be empty");
    setDuration(durationSeconds);
    ++liveCount_;
}
"""))
    s.append(p("5.2 Inheritance", "h2"))
    s.append(p(
        "Inheritance expresses “is-a”. A song is a media item, so <font face='Courier'>Song</font> "
        "publicly derives from <font face='Courier'>Media</font> and reuses id, title, creator, "
        "duration, source, favourite and play-count. Only album, genre and year live on the derived "
        "class. The same pattern is used for podcast episodes (show, episode number) and audiobooks "
        "(narrator, chapter). Protected helpers "
        "<font face='Courier'>requireText</font> and <font face='Courier'>optionalText</font> are "
        "inherited so every subclass validates strings the same way."
    ))
    s.append(p(
        "A second hierarchy sits under <font face='Courier'>std::runtime_error</font>: "
        "<font face='Courier'>AppException</font> adds an HTTP status, and seven concrete types "
        "override <font face='Courier'>type()</font>. One <font face='Courier'>catch (const AppException&amp;)</font> "
        "in <font face='Courier'>PlaylistApp::handle</font> is enough to turn any of them into a JSON "
        "error body."
    ))
    s.append(p("5.3 Polymorphism", "h2"))
    s.append(p(
        "Polymorphism is the reason a playlist can hold mixed types. "
        "<font face='Courier'>Media</font> declares pure virtual "
        "<font face='Courier'>type()</font>, <font face='Courier'>typeLabel()</font>, "
        "<font face='Courier'>subtitle()</font> and <font face='Courier'>writeFields()</font>, and "
        "overridable <font face='Courier'>m3uLabel()</font> and <font face='Courier'>matches()</font>. "
        "<font face='Courier'>Playlist::toM3u</font> walks <font face='Courier'>vector&lt;shared_ptr&lt;Media&gt;&gt;</font> "
        "and calls <font face='Courier'>toM3uEntry()</font> on the base; each derived class supplies "
        "its own label, so a song is written as “Roary - Odyssey” and a podcast as "
        "“Compile Time - Ep. 12: Why Virtual Functions Matter” without a "
        "<font face='Courier'>dynamic_cast</font> in the exporter."
    ))
    s.append(p(
        "<font face='Courier'>MediaFactory::create</font> is the other polymorphic seam: given a JSON "
        "object it returns a <font face='Courier'>shared_ptr&lt;Media&gt;</font> to the right subclass, "
        "or throws if the type is unknown. Operators are overloaded on the domain types: "
        "<font face='Courier'>Playlist::operator+=</font> appends a track, "
        "<font face='Courier'>operator[]</font> returns a const Media reference, "
        "<font face='Courier'>Media::operator&lt;</font> orders by title, and "
        "<font face='Courier'>operator&lt;&lt;</font> prints a readable line for debugging and tests."
    ))
    s.append(code_block("""
std::string Playlist::toM3u() const {
    if (tracks_.empty()) throw EmptyPlaylistException(name_);
    std::string out = "#EXTM3U\\n#PLAYLIST:" + name_ + "\\n";
    for (const auto& media : tracks_) out += media->toM3uEntry();
    return out;
}
"""))
    s.append(p("5.4 Standard Template Library", "h2"))
    s.append(p(
        "The STL is used as the default data layer, not as a demonstration of one container. "
        "The library keeps an <font face='Courier'>unordered_map&lt;string, shared_ptr&lt;Media&gt;&gt;</font> "
        "for O(1) lookup and a <font face='Courier'>vector&lt;string&gt;</font> for insertion order. "
        "Playlists are a <font face='Courier'>vector</font> of shared pointers so the same song can "
        "appear twice. <font face='Courier'>Registry&lt;T&gt;</font> is a class template parameterised "
        "on the stored type; it is instantiated as <font face='Courier'>Registry&lt;Playlist&gt;</font>."
    ))
    s.append(p(
        "Algorithms do the rest: <font face='Courier'>copy_if</font> and a predicate implement search "
        "and type filters; <font face='Courier'>stable_sort</font> implements the five sort keys; "
        "<font face='Courier'>rotate</font> implements drag-to-reorder; <font face='Courier'>shuffle</font> "
        "randomises a playlist; <font face='Courier'>accumulate</font> totals duration; "
        "<font face='Courier'>remove_if</font> / <font face='Courier'>replace_if</font> update tracks "
        "when media is deleted or edited; <font face='Courier'>any_of</font> detects a duplicate source; "
        "<font face='Courier'>transform</font> builds the ordered snapshot; "
        "<font face='Courier'>mismatch</font> prevents path traversal when serving static files. "
        "<font face='Courier'>std::filesystem</font> walks folders. "
        "<font face='Courier'>std::thread</font> and <font face='Courier'>std::mutex</font> run the server."
    ))
    s.append(p("5.5 Exception handling", "h2"))
    s.append(Anchor("tab-ex"))
    s.append(p(
        "Every user-visible failure is an object. The hierarchy and the status each type maps to are "
        "shown in Table 2. <font face='Courier'>PlaylistApp::handle</font> catches "
        "<font face='Courier'>AppException</font> first (typed JSON error) and "
        "<font face='Courier'>std::exception</font> second (unexpected 500). The browser prints the "
        "exception type next to the message, so a failed add is labelled "
        "<font face='Courier'>InvalidMediaException</font> rather than a generic “error”."
    ))
    exceptions = [
        ["Exception", "HTTP", "Typical cause"],
        ["InvalidMediaException", "400", "Empty title, bad year, missing file, unknown type."],
        ["BadRequestException", "400", "Malformed JSON, missing field, index out of range."],
        ["NotFoundException", "404", "Unknown media id, playlist id, or scan folder."],
        ["MethodNotAllowedException", "405", "POST/PUT/DELETE on a static page."],
        ["DuplicateException", "409", "Same id or same source path already in the library."],
        ["EmptyPlaylistException", "422", "Export requested on a playlist with zero tracks."],
        ["StorageException", "500", "library.json cannot be read or written."],
    ]
    s.append(table(exceptions, [52 * mm, 18 * mm, 90 * mm]))
    s.append(p("Table 2. Exception hierarchy and the HTTP status each type reports.", "caption"))
    s.append(PageBreak())
    return s


def ch6():
    s = chapter_head(6, "Implementation Details and code", "ch6")
    s.append(p("6.1 Language, tools and layout", "h2"))
    s.append(p(
        "The server is C++17, compiled with <font face='Courier'>c++ -std=c++17 -O2 -Wall -Wextra "
        "-Wpedantic</font> and linked with pthreads. There is no CMake dependency and no external "
        "package. A Makefile produces <font face='Courier'>bin/playlist-builder</font> and "
        "<font face='Courier'>bin/tests</font>. Headers live in <font face='Courier'>include/</font>, "
        "implementations in <font face='Courier'>src/</font>, the player in "
        "<font face='Courier'>public/</font>, and tests in <font face='Courier'>tests/test_core.cpp</font>. "
        "A Dockerfile builds the same binary on Debian for optional deployment."
    ))
    s.append(p("6.2 Runtime architecture", "h2"))
    s.append(p(
        "On start, <font face='Courier'>main</font> parses <font face='Courier'>--port</font>, "
        "<font face='Courier'>--host</font>, <font face='Courier'>--data</font>, "
        "<font face='Courier'>--public</font> and <font face='Courier'>--samples</font>. "
        "<font face='Courier'>PlaylistApp</font> either loads <font face='Courier'>data/library.json</font> "
        "or, on first run, asks <font face='Courier'>Seeder</font> to synthesise twelve WAV clips and "
        "two playlists. <font face='Courier'>HttpServer</font> accepts connections on a thread each, "
        "parses the request, and calls <font face='Courier'>PlaylistApp::handle</font>. Mutations take "
        "a mutex and call <font face='Courier'>Storage::save</font>, which writes a temporary file and "
        "renames it so a crash cannot leave a truncated JSON document."
    ))
    s.append(p(
        "Local audio is served from <font face='Courier'>GET /stream/:id</font> with HTTP Range support "
        "so the browser can seek. Remote items keep their original URL. Playlist export is "
        "<font face='Courier'>GET /api/playlists/:id/export.m3u</font> with a "
        "<font face='Courier'>Content-Disposition</font> filename derived from the playlist name."
    ))
    s.append(p("6.3 User interface", "h2"))
    s.append(p(
        "The page is a pair of 424-pixel panels on desktop and a single full-screen panel on a phone. "
        "The left panel is now playing: title, heart, circular glass disc with a progress arc, transport "
        "controls and a glass navigation bar. The right panel is the library, playlists, add-media form "
        "or activity log, with a mini player when something is playing. Album art is a deterministic "
        "SVG generated from the item id, so the demo does not need image files. Figures 2 to 6 are "
        "screenshots of the running server at <font face='Courier'>http://127.0.0.1:8080</font>."
    ))
    s.append(figure("fig-now-playing.png",
                    "Figure 2. Now playing (Skin) beside the library list.", 6.4 * inch, "fig-ui"))
    s.append(figure("fig-playlists.png",
                    "Figure 3. Playlist list with the mini player and progress fill.", 6.4 * inch, "fig-pl"))
    s.append(figure("fig-playlist-detail.png",
                    "Figure 4. Playlist detail: play, shuffle, export .m3u, duplicate, rename, delete.",
                    6.4 * inch, "fig-pld"))
    s.append(figure("fig-add-media.png",
                    "Figure 5. Add-media form. Extra fields change with Song, Podcast or Audiobook.",
                    6.4 * inch, "fig-add"))
    s.append(figure("fig-error.png",
                    "Figure 6. Submitting an empty title returns InvalidMediaException in the form.",
                    6.4 * inch, "fig-err"))
    s.append(p("6.4 Selected implementation", "h2"))
    s.append(p(
        "Derived constructors call the base constructor, then validate their own fields. "
        "Virtual <font face='Courier'>subtitle()</font> is what the now-playing chip and the library "
        "row both display:"
    ))
    s.append(code_block("""
std::string Song::subtitle() const {
    std::string text = creator();
    if (!album_.empty()) text += " · " + album_;
    if (year_ != 0) text += " (" + std::to_string(year_) + ")";
    return text;
}

std::string PodcastEpisode::subtitle() const {
    return show_ + " · Episode " + std::to_string(episode_);
}

std::string Audiobook::subtitle() const {
    std::string text = creator() + " · Chapter " + std::to_string(chapter_);
    if (!narrator_.empty()) text += " · read by " + narrator_;
    return text;
}
"""))
    s.append(p(
        "Library search is a filter plus a virtual match, then a stable sort. The type of each object "
        "is not tested with <font face='Courier'>if (song)</font> except when the user asked for a "
        "type chip; matching itself is polymorphic:"
    ))
    s.append(code_block("""
std::vector<std::shared_ptr<Media>> Library::search(
        const std::string& query, MediaFilter filter, SortKey sort) const {
    const std::string needle = toLower(query);
    auto keep = [&](const std::shared_ptr<Media>& m) {
        switch (filter) {
            case MediaFilter::Songs:      if (m->type() != "song") return false; break;
            case MediaFilter::Podcasts:   if (m->type() != "podcast") return false; break;
            case MediaFilter::Audiobooks: if (m->type() != "audiobook") return false; break;
            case MediaFilter::Favorites:  if (!m->favorite()) return false; break;
            case MediaFilter::All: break;
        }
        return m->matches(needle);
    };
    // copy_if into a vector, then stable_sort by the chosen key
    ...
}
"""))
    s.append(p(
        "The web layer does not invent its own error strings. It rethrows nothing: it catches the "
        "domain exception and copies <font face='Courier'>type()</font>, <font face='Courier'>what()</font> "
        "and <font face='Courier'>status()</font> into JSON."
    ))
    s.append(code_block("""
} catch (const AppException& e) {
    return errorResponse(e.type(), e.what(), e.status());
}
"""))
    s.append(p(
        "How to run the project from the folder <font face='Courier'>sem3-code/oop/playlist-builder</font>:"
    ))
    s.append(code_block("""
make            # builds bin/playlist-builder
make test       # 27 unit tests
make run        # http://127.0.0.1:8080
"""))
    s.append(PageBreak())
    return s


def ch7():
    s = chapter_head(7, "Test Cases and Results", "ch7")
    s.append(p(
        "Automated tests live in <font face='Courier'>tests/test_core.cpp</font> and are built with "
        "<font face='Courier'>make test</font>. They construct real domain objects, including short WAV "
        "files written by <font face='Courier'>SampleGenerator</font>, and they assert both the happy "
        "path and the exception type. All 27 cases passed on Apple clang 21 (macOS) as recorded below."
    ))
    s.append(Anchor("tab-tests"))
    rows = [
        ["ID", "Test case", "Expected", "Result"],
        ["TC01", "Song constructor stores trimmed title and year", "title Odyssey, year 2024", "PASS"],
        ["TC02", "Empty title rejected", "InvalidMediaException", "PASS"],
        ["TC03", "Negative duration rejected", "InvalidMediaException", "PASS"],
        ["TC04", "Year 1800 rejected", "InvalidMediaException", "PASS"],
        ["TC05", "Podcast episode 0 rejected", "InvalidMediaException", "PASS"],
        ["TC06", "Virtual subtitle() per derived class", "three different strings", "PASS"],
        ["TC07", "MediaFactory builds Audiobook", "dynamic_cast succeeds", "PASS"],
        ["TC08", "Unknown type rejected", "InvalidMediaException", "PASS"],
        ["TC09", "Search matches Song album", "one hit", "PASS"],
        ["TC10", "Filter songs, sort by title", "alpha then Zeta", "PASS"],
        ["TC11", "Duplicate id and source rejected", "DuplicateException", "PASS"],
        ["TC12", "Missing id", "NotFoundException", "PASS"],
        ["TC13", "Playlist +=, duration, operator[]", "size 2, 150 s", "PASS"],
        ["TC14", "move() uses std::rotate", "new order", "PASS"],
        ["TC15", "Out-of-range move", "BadRequestException", "PASS"],
        ["TC16", "Empty playlist export", "EmptyPlaylistException", "PASS"],
        ["TC17", "M3U uses each type’s label", "EXTINF song and podcast lines", "PASS"],
        ["TC18", "Copy constructor independent", "original size unchanged", "PASS"],
        ["TC19", "removeMedia drops every occurrence", "two removed", "PASS"],
        ["TC20", "Blank playlist name", "BadRequestException", "PASS"],
        ["TC21", "JSON round-trip and bad input", "parse / BadRequestException", "PASS"],
        ["TC22", "Storage save/load", "library and playlist restored", "PASS"],
        ["TC23", "Corrupt save file", "StorageException", "PASS"],
        ["TC24", "Folder scanner Artist - Title + WAV duration", "Nova Lane / Starfall / 3 s", "PASS"],
        ["TC25", "Missing scan folder", "NotFoundException", "PASS"],
        ["TC26", "Virtual destructor via base pointer", "liveCount returns to baseline", "PASS"],
        ["TC27", "operator&lt;&lt; printable line", "[Song] Skin - … (2:05)", "PASS"],
    ]
    s.append(table(rows, [16 * mm, 62 * mm, 58 * mm, 24 * mm]))
    s.append(p("Table 3. Unit test cases and results (<font face='Courier'>make test</font>: 27 tests, 0 failed).", "caption"))
    s.append(p("7.1 Manual checks on the running server", "h2"))
    s.append(p(
        "With <font face='Courier'>bin/playlist-builder</font> listening on port 8080 the following "
        "browser and HTTP checks were performed:"
    ))
    s.append(bullets([
        "Library lists 12 items (8 songs, 2 podcasts, 2 audiobook chapters) and 3 favourites.",
        "Clicking Skin starts playback, updates the document title, lights the progress arc and shows the mini player.",
        "Playlists Late Night Coding and Focus Flow open, shuffle, and export "
        "<font face='Courier'>Late-Night-Coding.m3u</font> with EXTINF lines that differ by type.",
        "<font face='Courier'>POST /api/media</font> with an empty title returns HTTP 400 and "
        "<font face='Courier'>{\"type\":\"InvalidMediaException\",\"message\":\"Title cannot be empty\"}</font>; "
        "the add form shows the same badge (Figure 6).",
        "<font face='Courier'>GET /api/playlists/p3/export.m3u</font> on a newly created empty playlist "
        "returns HTTP 422 <font face='Courier'>EmptyPlaylistException</font>.",
        "Search, type chips, favourite heart, next/previous and folder-scan path validation all respond without a crash.",
    ]))
    s.append(PageBreak())
    return s


def ch8():
    s = chapter_head(8, "Individual Contributions", "ch8")
    s.append(p(
        "This mini project was completed jointly by Harsh Gidwani, Ansh Thakare, Darshan Nayak and "
        "Anushka Wani, under the internal guidance of Prof. Poonam Bhosle. Table 4 lists each member."
    ))
    s.append(Anchor("tab-contrib"))
    rows = [
        ["SrNo", "Project member", "Contribution"],
        ["1", "Harsh Gidwani", "Design, C++ implementation, browser UI, testing and this report (joint)."],
        ["2", "Ansh Thakare", "Design, C++ implementation, browser UI, testing and this report (joint)."],
        ["3", "Darshan Nayak", "Design, C++ implementation, browser UI, testing and this report (joint)."],
        ["4", "Anushka Wani", "Design, C++ implementation, browser UI, testing and this report (joint)."],
    ]
    s.append(table(rows, [18 * mm, 40 * mm, 102 * mm]))
    s.append(p("Table 4. Individual contributions.", "caption"))
    s.append(p(
        "The four members shared the class design, the exception cases, the user interface and the final demonstration."
    ))
    s.append(PageBreak())
    return s


def ch9():
    s = chapter_head(9, "Conclusion and Future Scope", "ch9")
    s.append(p("9.1 Conclusion", "h2"))
    s.append(p(
        "Playlist Builder shows that the five OOP topics required by the course can sit inside one "
        "program that someone would actually open. Constructors refuse bad data. Inheritance shares "
        "the fields that songs, podcasts and audiobooks have in common. Polymorphism writes the right "
        "playlist line and the right subtitle through a base pointer. The STL holds, searches and "
        "reorders those objects. Exceptions turn every failure into a message the browser can show. "
        "Twenty-seven automated tests and a working player at "
        "<font face='Courier'>http://127.0.0.1:8080</font> support those claims."
    ))
    s.append(p("9.2 Future scope", "h2"))
    s.append(numbered([
        "Read ID3 / Vorbis tags so scanned files pick up album art and duration without a WAV-only parser.",
        "Persist playlists as separate .m3u files that the OS file manager can open, in addition to the JSON save.",
        "Add a <font face='Courier'>Video</font> derived class if the course extends the hierarchy later.",
        "Replace the in-process JSON file with SQLite while keeping the same Media interface.",
        "Package a signed macOS build (or a Docker image on a free host) so the player can be opened from a bookmark without compiling.",
    ]))
    s.append(Spacer(1, 16 * mm))
    s.append(p("— End of report —", "center"))
    return s


def build_story():
    story = []
    story.extend(cover())
    story.extend(certificate())
    story.extend(abstract_page())
    story.extend(figures_page())
    story.extend(contents_page())
    story.extend(ch1())
    story.extend(ch2())
    story.extend(ch3())
    story.extend(ch4())
    story.extend(ch5())
    story.extend(ch6())
    story.extend(ch7())
    story.extend(ch8())
    story.extend(ch9())
    return story


def make_doc(path):
    doc = BaseDocTemplate(
        str(path),
        pagesize=A4,
        title=f"Mini Project Report on {FULL_TITLE}",
        author=STUDENT,
        subject=f"{COURSE} ({COURSE_CODE})",
    )
    margin = 22 * mm
    frame = Frame(margin, 20 * mm, PAGE_W - 2 * margin, PAGE_H - 32 * mm, id="normal")
    cover_frame = Frame(margin, 18 * mm, PAGE_W - 2 * margin, PAGE_H - 30 * mm, id="cover")
    doc.addPageTemplates([
        PageTemplate(id="cover", frames=[cover_frame], onPage=footer_none),
        PageTemplate(id="front", frames=[frame], onPage=footer_roman),
        PageTemplate(id="body", frames=[frame], onPage=footer_arabic),
    ])
    return doc


def main():
    doc = make_doc(OUT)
    # First pass: populate page_map. Second pass: write TOC with real numbers.
    for _ in range(2):
        doc = make_doc(OUT)
        doc.build(build_story())
    print(f"Wrote {OUT} ({OUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
