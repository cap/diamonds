const char* global_balls[7] = {
  ".###."
  "#...#"
  "#...#"
  "#...#"
  ".###.",

  ".###."
  "#...#"
  "#.#.#"
  "#...#"
  ".###.",

  ".###."
  "#.#.#"
  "##.##"
  "#.#.#"
  ".###.",

  ".###."
  "#.#.#"
  "#.#.#"
  "#.#.#"
  ".###.",

  ".###."
  "#..##"
  "#..##"
  "###.#"
  ".###.",

  ".###."
  "##.##"
  "#...#"
  "##.##"
  ".###.",

  ".###."
  "#####"
  "#####"
  "#####"
  ".###.",
};


const char* global_tiles[19] = {
// empty
  "............"
  "............"
  "............"
  "............"
  "............"
  "............",
// border
  "############"
  "############"
  "############"
  "############"
  "############"
  "############",
// shadow
  ".##########."
  "############"
  "############"
  "############"
  "############"
  ".##########.",
// diamond
  ".####..####."
  "####....####"
  "###......###"
  "###......###"
  "####....####"
  ".####..####.",
// inert
  ".##########."
  "############"
  "##.####.####"
  "####.####.##"
  "############"
  ".##########.",
// key
  ".##########."
  "##...#######"
  "#.###......#"
  "#.###.##.#.#"
  "##...###.#.#"
  ".##########.",
// lock
  ".##########."
  "#####..#####"
  "####.##.####"
  "###......###"
  "###......###"
  ".##########.",
// kill
  ".##########."
  "#####..#####"
  "##........##"
  "##........##"
  "#####..#####"
  ".##########.",
// switch
  ".##.#######."
  "##..####.###"
  "#.....##..##"
  "##..##.....#"
  "###.####..##"
  ".#######.##.",
// color a
  ".##########."
  "#..........#"
  "#..........#"
  "#..........#"
  "#..........#"
  ".##########.",
// paint a
  ".##########."
  "#.......####"
  "#.......####"
  "#.......####"
  "#.......####"
  ".##########.",
// color b
  ".##########."
  "#..........#"
  "#.########.#"
  "#.########.#"
  "#..........#"
  ".##########.",
// paint b
  ".##########."
  "#.......####"
  "#.#####.####"
  "#.#####.####"
  "#.......####"
  ".##########.",
// color c
  ".##########."
  "##.#.#.#.#.#"
  "#.#.#.#.#.##"
  "##.#.#.#.#.#"
  "#.#.#.#.#.##"
  ".##########.",
// paint c
  ".##########."
  "##.#.#..####"
  "#.#.#.#.####"
  "##.#.#..####"
  "#.#.#.#.####"
  ".##########.",
// color d
  ".##########."
  "#.#.#..#.#.#"
  "#.#.#..#.#.#"
  "#.#.#..#.#.#"
  "#.#.#..#.#.#"
  ".##########.",
// paint d
  ".##########."
  "#.#.#...####"
  "#.#.#...####"
  "#.#.#...####"
  "#.#.#...####"
  ".##########.",
// color e
  ".##########."
  "#..##..##..#"
  "#..##..##..#"
  "###..##..###"
  "###..##..###"
  ".##########.",
// paint e
  ".##########."
  "#..##...####"
  "#..##...####"
  "###..##.####"
  "###..##.####"
  ".##########.",
};
