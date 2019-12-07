var fs = require("fs");

var table = new Array();

function Results()
{
    this.line_count = 0;
    this.word_count = 0;
    this.char_count = 0;
    this.was_space = true;
    this.update = function (line_count, word_count, char_count, was_space) {
        this.line_count += line_count;
        this.word_count += word_count;
        this.char_count += char_count;
        this.was_space = was_space;
        return this;
    }
    this.update2 = function (s) {
        return this.update(s.line_count, s.word_count, s.char_count, s.was_space);
    }
}
function isspace(c) {
    return (9 <= c && c <= 13) || (c == 32);
}

function parse_chunk(buf, length, state)
{
    var was_space = state.was_space;
    var line_count = 0;
    var word_count = 0;

    for (var i=0; i<length; i++) {
        var c = buf[i];
        var is_space = table[c];
        line_count += (c == 10) ? 1 : 0;
        word_count += (!is_space && was_space) ? 1 : 0;
        was_space = is_space;
    }

    return state.update(line_count, word_count, length, was_space);
}

function parse_file(fd, filename)
{
    var state = new Results();
    var buf = new Buffer(65536);

    for (;;) {
        var length = fs.readSync(fd, buf, 0, 65536);
        if (length == 0)
            break;
        state = parse_chunk(buf, length, state);
    }
    
    console.log(state.line_count + " " + state.word_count + " " + state.char_count + " " + filename);
    return state;
}

function main(argv)
{
    var file_count = 0;
    var total = new Results();

    for (var i=0; i<256; i++)
        table[i] = isspace(i);

    for (var i in argv) {
        var filename = argv[i];
        if (i < 2)
            continue;
        try {
            var fd = fs.openSync(filename, "r");
            total.update2(parse_file(fd, filename));
            fs.closeSync(fd);
            file_count++;
        } catch (err) {
            console.log(err);
            break;
        }
    }
    if (file_count == 0)
        total.update2(parse_file(0, ""));
    if (file_count > 1)
        console.log(total.line_count + " " + total.word_count + " " + total.char_count + " " + "total");

}

main(process.argv);

