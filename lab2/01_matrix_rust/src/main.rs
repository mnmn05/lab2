// 실습 2-2: 두 행렬의 크기를 입력받아 Vec으로 동적 할당 후 덧셈
use std::io::{self, BufRead, Write};

type Matrix = Vec<Vec<i64>>;

/// 표준입력에서 공백으로 구분된 정수 토큰을 순서대로 꺼내 주는 리더
struct Reader {
    tokens: Vec<String>,
    lines: io::Lines<io::StdinLock<'static>>,
}

impl Reader {
    fn new() -> Self {
        Reader { tokens: Vec::new(), lines: io::stdin().lock().lines() }
    }

    fn next<T: std::str::FromStr>(&mut self, prompt: &str) -> T {
        loop {
            if self.tokens.is_empty() {
                print!("{}", prompt);
                io::stdout().flush().unwrap();
                match self.lines.next() {
                    Some(Ok(line)) => {
                        self.tokens = line.split_whitespace().rev().map(String::from).collect();
                    }
                    _ => {
                        eprintln!("\n입력이 끝났습니다.");
                        std::process::exit(1);
                    }
                }
                continue;
            }
            let tok = self.tokens.pop().unwrap();
            match tok.parse::<T>() {
                Ok(v) => return v,
                Err(_) => {
                    println!("  '{}' 은(는) 올바른 값이 아닙니다. 무시하고 다음 값을 사용합니다.", tok);
                }
            }
        }
    }
}

fn read_matrix(rd: &mut Reader, name: &str, rows: usize, cols: usize) -> Matrix {
    println!("행렬 {} ({} x {}) 원소를 입력하세요:", name, rows, cols);
    let mut m: Matrix = Vec::with_capacity(rows);
    for i in 0..rows {
        let mut row = Vec::with_capacity(cols);
        for j in 0..cols {
            row.push(rd.next::<i64>(&format!("  {}[{}][{}] = ", name, i, j)));
        }
        m.push(row);
    }
    m
}

fn add(a: &Matrix, b: &Matrix) -> Result<Matrix, String> {
    if a.len() != b.len() || a.iter().zip(b).any(|(r1, r2)| r1.len() != r2.len()) {
        return Err("두 행렬의 크기가 달라서 더할 수 없습니다.".to_string());
    }
    Ok(a.iter()
        .zip(b)
        .map(|(r1, r2)| r1.iter().zip(r2).map(|(x, y)| x + y).collect())
        .collect())
}

fn print_matrix(name: &str, m: &Matrix) {
    println!("{} =", name);
    for row in m {
        let s: Vec<String> = row.iter().map(|v| format!("{:6}", v)).collect();
        println!("  [{} ]", s.join(""));
    }
}

fn main() {
    let mut rd = Reader::new();
    let r1: usize = rd.next("행렬 A의 행 수: ");
    let c1: usize = rd.next("행렬 A의 열 수: ");
    let r2: usize = rd.next("행렬 B의 행 수: ");
    let c2: usize = rd.next("행렬 B의 열 수: ");

    if r1 == 0 || c1 == 0 || r2 == 0 || c2 == 0 {
        eprintln!("행/열 크기는 1 이상이어야 합니다.");
        std::process::exit(1);
    }
    if r1 != r2 || c1 != c2 {
        eprintln!("두 행렬의 크기({}x{}, {}x{})가 달라 덧셈을 할 수 없습니다.", r1, c1, r2, c2);
        std::process::exit(1);
    }

    let a = read_matrix(&mut rd, "A", r1, c1);
    let b = read_matrix(&mut rd, "B", r2, c2);

    match add(&a, &b) {
        Ok(c) => {
            println!();
            print_matrix("A", &a);
            print_matrix("B", &b);
            print_matrix("A + B", &c);
        }
        Err(e) => eprintln!("{}", e),
    }
    // Vec은 스코프를 벗어나면 자동으로 해제된다 (C의 free 불필요)
}
