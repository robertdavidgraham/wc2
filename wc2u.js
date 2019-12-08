var fs = require("fs");
const WAS_SPACE = 0;
const NEW_LINE = 1;
const NEW_WORD = 2;
const WAS_WORD = 3;

var table;

function table_init()
{
    var i;
    var spaces = [9,10,11,12,13,32];

    table = Buffer.alloc(4 * 256);
    
    /* Transitions when not a space */
    for (i=0; i<256; i++) {
        var c = i;
        table[WAS_SPACE* 256 + c] = NEW_WORD;
        table[NEW_LINE * 256 + c] = NEW_WORD;
        table[NEW_WORD * 256 + c] = WAS_WORD;
        table[WAS_WORD * 256 + c] = WAS_WORD;
    }

    /* Transitions when space */
    for (i in spaces) {
        var c = spaces[i];
        table[WAS_SPACE* 256 + c] = WAS_SPACE;
        table[NEW_LINE * 256 + c] = WAS_SPACE;
        table[NEW_WORD * 256 + c] = WAS_SPACE;
        table[WAS_WORD * 256 + c] = WAS_SPACE;
    }

    /* Transitions when newline \n */
    table[WAS_SPACE* 256 + 10] = NEW_LINE;
    table[NEW_LINE * 256 + 10] = NEW_LINE;
    table[NEW_WORD * 256 + 10] = NEW_LINE;
    table[WAS_WORD * 256 + 10] = NEW_LINE;

}
table_init();



function Results()
{
    this.line_count = 0;
    this.word_count = 0;
    this.char_count = 0;
    this.state = WAS_SPACE;
    this.update = function (line_count, word_count, char_count, state) {
        this.line_count += line_count;
        this.word_count += word_count;
        this.char_count += char_count;
        this.state = state;
        return this;
    }
    this.update2 = function (s) {
        return this.update(s.line_count, s.word_count, s.char_count, s.state);
    }
}

function parse_chunk(buf, length, results)
{
    var state = results.state;
    var c;
    var i;
    var counts = [0,0,0,0];

    for (i=0; i<length; i++) {
        c = buf[i];
        state = table[state  * 256 + c];
        counts[state]++;
    }

    return results.update(counts[NEW_LINE], counts[NEW_WORD], length, state);
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
    var results = new Results();
    var buf = Buffer.alloc(65536);

    for (;;) {
        var length = fs.readSync(fd, buf, 0, 65536);
        if (length == 0)
            break;
        results = parse_chunk(buf, length, results);
    }
    
    print_results(results, cfg, filename);
    return results;
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

