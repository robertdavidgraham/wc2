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
    var c;
    var is_space;
    var i;

    for (i=0; i<length; i++) {
        c = buf[i];
        is_space = table[c];
        line_count += (c == 10) ? 1 : 0;
        word_count += (!is_space && was_space) ? 1 : 0;
        was_space = is_space;
    }

    return state.update(line_count, word_count, length, was_space);
}

function print_results(results, cfg, filename)
{
    var str = "";
    if (cfg.is_line_count)
        str = str + " " + results.line_count;
    if (cfg.is_word_count)
        str = str + " " + results.word_count;
    if (cfg.is_char_count)
        str = str + " " + results.char_count;
    if (filename && filename.length > 0)
        str = str + " " + filename;
    console.log(str);
}

function parse_file(fd, filename, cfg)
{
    var state = new Results();
    var buf = Buffer.alloc(65536);

    for (;;) {
        var length = fs.readSync(fd, buf, 0, 65536);
        if (length == 0)
            break;
        state = parse_chunk(buf, length, state);
    }
    
    print_results(state, cfg, filename);
    return state;
}


function print_help()
{
    console.log("usage: wc [-clmw] [file ...]");
    process.exit(1);
}

function parse_configuration(argv)
{
    var cfg = {is_line_count:false, is_word_count:false, is_char_count:false};
    
    for (var i in argv) {
        if (argv[i].charAt(0) == '-') {
            for (var j=1; j<argv[i].length; j++) {
                switch (argv[i].charAt(j)) {
                    case 'l': cfg.is_line_count = true; break;
                    case 'w': cfg.is_word_count = true; break;
                    case 'c': cfg.is_char_count = true; break;
                    case '?': print_help(); break;
                    case 'h': print_help(); break;
                    case '-':
                        if (argv[i].charAt(j+1) == 'h')
                            print_help();
                        else {
                            console.log("[-] unknown option: " + argv[i]);
                            process.exit(1);
                        }
                        break;
                    default:
                        console.log("[-] unknown option: -" + argv[i].charAt(i));
                        process.exit(1);
                        break;
                }
            }
        }
    }

    if (cfg.is_line_count == false && cfg.is_word_count == false && cfg.is_char_count == false)
        cfg = {is_line_count: true, is_word_count: true, is_char_count: true};

    return cfg;
}

function main(argv)
{
    var file_count = 0;
    var total = new Results();
    
    /* setup character table to determine spaces */
    for (var i=0; i<256; i++)
        table[i] = isspace(i);

    /* Parse configuration */
    var cfg = parse_configuration(argv);

    /* Parse all the files */
    for (var i in argv) {
        var filename = argv[i];
        if (i < 2)
            continue;
        if (argv[i].length == 0)
            continue;
        if (argv[i].charAt(0) == '-')
            continue;
        try {
            var fd = fs.openSync(filename, "r");
            total.update2(parse_file(fd, filename, cfg));
            fs.closeSync(fd);
            file_count++;
        } catch (err) {
            if (err.syscall == 'open' && err.code == 'ENOENT') {
                console.log("[-] " + err.path + ": No such file or directory");
            } else 
                console.log(err);
            process.exit(1);
            break;
        }
    }
    if (file_count == 0)
        total.update2(parse_file(0, "", cfg));
    if (file_count > 1)
        print_results(total, cfg, "total");

}

main(process.argv);

