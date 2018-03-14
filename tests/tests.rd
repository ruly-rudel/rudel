;; Testing evaluation of arithmetic operations
(+ 1 2)
;=>3

(+ 5 (* 2 3))
;=>11

(- (+ 5 (* 2 3)) 3)
;=>8

(/ (- (+ 5 (* 2 3)) 3) 4)
;=>2

(/ (- (+ 515 (* 87 311)) 302) 27)
;=>1010

(* -3 6)
;=>-18

(/ (- (+ 515 (* -87 311)) 296) 27)
;=>-994

(abc 1 2 3)
; .*\'abc\' not found.*

;; Testing empty list
()
;=>nil

;; Testing REPL_ENV
(+ 1 2)
;=>3
(/ (- (+ 5 (* 2 3)) 3) 4)
;=>2


;; Testing def!
(def! x 3)
;=>3
x
;=>3
(def! x 4)
;=>4
x
;=>4
(def! y (+ 1 7))
;=>8
y
;=>8

;; Verifying symbols are case-sensitive
(def! mynum 111)
;=>111
(def! MYNUM 222)
;=>222
mynum
;=>111
MYNUM
;=>222

;; Check env lookup non-fatal error
(abc 1 2 3)
; .*\'abc\' not found.*
;; Check that error aborts def!
(def! w 123)
(def! w (abc))
w
;=>123

;; Testing let*
(let* (z 9) z)
;=>9
(let* (x 9) x)
;=>9
x
;=>4
(let* (z (+ 2 3)) (+ 1 z))
;=>6
(let* (p (+ 2 3) q (+ 2 p)) (+ p q))
;=>12
(def! y (let* (z 7) z))
y
;=>7

;; Testing outer environment
(def! a 4)
;=>4
(let* (q 9) q)
;=>9
(let* (q 9) a)
;=>4
(let* (z 2) (let* (q 9) a))
;=>4
(let* (x 4) (def! a 5))
;=>5
a
;=>4

;; -----------------------------------------------------


;; Testing list functions
(list)
;=>nil
(list? (list))
;=>t
(empty? (list))
;=>t
(empty? (list 1))
;=>nil
(list 1 2 3)
;=>(1 2 3)
(count (list 1 2 3))
;=>3
(count (list))
;=>0
(count nil)
;=>0
(if (> (count (list 1 2 3)) 3) "yes" "no")
;=>"no"
(if (>= (count (list 1 2 3)) 3) "yes" "no")
;=>"yes"


;; Testing if form
(if t 7 8)
;=>7
(if nil 7 8)
;=>8
(if t (+ 1 7) (+ 1 8))
;=>8
(if nil (+ 1 7) (+ 1 8))
;=>9
(if nil 7 8)
;=>8
(if 0 7 8)
;=>7
(if "" 7 8)
;=>7
(if (list) 7 8)
;=>8
(if (list 1 2 3) 7 8)
;=>7
(= (list) nil)
;=>t


;; Testing 1-way if form
(if nil (+ 1 7))
;=>nil
(if nil 8 7)
;=>7
(if t (+ 1 7))
;=>8


;; Testing basic conditionals
(= 2 1)
;=>nil
(= 1 1)
;=>t
(= 1 2)
;=>nil
(= 1 (+ 1 1))
;=>nil
(= 2 (+ 1 1))
;=>t
(= nil 1)
;=>nil
(= nil nil)
;=>t

(> 2 1)
;=>t
(> 1 1)
;=>nil
(> 1 2)
;=>nil

(>= 2 1)
;=>t
(>= 1 1)
;=>t
(>= 1 2)
;=>nil

(< 2 1)
;=>nil
(< 1 1)
;=>nil
(< 1 2)
;=>t

(<= 2 1)
;=>nil
(<= 1 1)
;=>t
(<= 1 2)
;=>t


;; Testing equality
(= 1 1)
;=>t
(= 0 0)
;=>t
(= 1 0)
;=>nil
(= "" "")
;=>t
(= "abc" "abc")
;=>t
(= "abc" "")
;=>nil
(= "" "abc")
;=>nil
(= "abc" "def")
;=>nil
(= "abc" "ABC")
;=>nil
(= t t)
;=>t
(= nil nil)
;=>t
(= nil nil)
;=>t

(= (list) (list))
;=>t
(= (list 1 2) (list 1 2))
;=>t
(= (list 1) (list))
;=>nil
(= (list) (list 1))
;=>nil
(= 0 (list))
;=>nil
(= (list) 0)
;=>nil
(= (list) "")
;=>nil
(= "" (list))
;=>nil


;; Testing builtin and user defined functions
(+ 1 2)
;=>3
( (fn* (a b) (+ b a)) 3 4)
;=>7
( (fn* () 4) )
;=>4

( (fn* (f x) (f x)) (fn* (a) (+ 1 a)) 7)
;=>8


;; Testing closures
( ( (fn* (a) (fn* (b) (+ a b))) 5) 7)
;=>12

(def! gen-plus5 (fn* () (fn* (b) (+ 5 b))))
(def! plus5 (gen-plus5))
(plus5 7)
;=>12

(def! gen-plusX (fn* (x) (fn* (b) (+ x b))))
(def! plus7 (gen-plusX 7))
(plus7 8)
;=>15

;; Testing do form
(do (prn "prn output1"))
; "prn output1"
;=>nil
(do (prn "prn output2") 7)
; "prn output2"
;=>7
(do (prn "prn output1") (prn "prn output2") (+ 1 2))
; "prn output1"
; "prn output2"
;=>3

(do (def! a 6) 7 (+ a 8))
;=>14
a
;=>6

;; Testing special form case-sensitivity
(def! DO (fn* (a) 7))
(DO 3)
;=>7

;; Testing recursive sumdown function
(def! sumdown (fn* (N) (if (> N 0) (+ N (sumdown  (- N 1))) 0)))
(sumdown 1)
;=>1
(sumdown 2)
;=>3
(sumdown 6)
;=>21


;; Testing recursive fibonacci function
(def! fib (fn* (N) (if (= N 0) 1 (if (= N 1) 1 (+ (fib (- N 1)) (fib (- N 2)))))))
(fib 1)
;=>1
(fib 2)
;=>2
(fib 4)
;=>5
;;; Too slow for bash, erlang, make and miniMAL
;;;(fib 10)
;;;;=>89


;>>> deferrable=True
;;
;; -------- Deferrable Functionality --------

;; Testing variable length arguments

( (fn* (& more) (count more)) 1 2 3)
;=>3
( (fn* (& more) (list? more)) 1 2 3)
;=>t
( (fn* (& more) (count more)) 1)
;=>1
( (fn* (& more) (count more)) )
;=>0
( (fn* (& more) (list? more)) )
;=>t
( (fn* (a & more) (count more)) 1 2 3)
;=>2
( (fn* (a & more) (count more)) 1)
;=>0
( (fn* (a & more) (list? more)) 1)
;=>t


;; Testing language defined not function
(not nil)
;=>t
(not t)
;=>nil
(not "a")
;=>nil
(not 0)
;=>nil


;; -----------------------------------------------------

;; Testing string quoting

""
;=>""

"abc"
;=>"abc"

"abc  def"
;=>"abc  def"

"\""
;=>"\""

"abc\ndef\nghi"
;=>"abc\ndef\nghi"

"abc\\def\\ghi"
;=>"abc\\def\\ghi"

"\\n"
;=>"\\n"

;; Testing pr-str

(pr-str)
;=>""

(pr-str "")
;=>"\"\""

(pr-str "abc")
;=>"\"abc\""

(pr-str "abc  def" "ghi jkl")
;=>"\"abc  def\" \"ghi jkl\""

(pr-str "\"")
;=>"\"\\\"\""

(pr-str (list 1 2 "abc" "\"") "def")
;=>"(1 2 \"abc\" \"\\\"\") \"def\""

(pr-str "abc\ndef\nghi")
;=>"\"abc\\ndef\\nghi\""

(pr-str "abc\\def\\ghi")
;=>"\"abc\\\\def\\\\ghi\""

(pr-str (list))
;=>"nil"

;; Testing str

(str)
;=>""

(str "")
;=>""

(str "abc")
;=>"abc"

(str "\"")
;=>"\""

(str 1 "abc" 3)
;=>"1abc3"

(str "abc  def" "ghi jkl")
;=>"abc  defghi jkl"

(str "abc\ndef\nghi")
;=>"abc\ndef\nghi"

(str "abc\\def\\ghi")
;=>"abc\\def\\ghi"

(str (list 1 2 "abc" "\"") "def")
;=>"(1 2 abc \")def"

(str (list))
;=>"nil"

;; Testing prn
(prn)
; 
;=>nil

(prn "")
; ""
;=>nil

(prn "abc")
; "abc"
;=>nil

(prn "abc  def" "ghi jkl")
; "abc  def" "ghi jkl"

(prn "\"")
; "\""
;=>nil

(prn "abc\ndef\nghi")
; "abc\ndef\nghi"
;=>nil

(prn "abc\\def\\ghi")
; "abc\\def\\ghi"
nil

(prn (list 1 2 "abc" "\"") "def")
; (1 2 "abc" "\"") "def"
;=>nil


;; Testing println
(println)
; 
;=>nil

(println "")
; 
;=>nil

(println "abc")
; abc
;=>nil

(println "abc  def" "ghi jkl")
; abc  def ghi jkl

(println "\"")
; "
;=>nil

(println "abc\ndef\nghi")
; abc
; def
; ghi
;=>nil

(println "abc\\def\\ghi")
; abc\def\ghi
;=>nil

(println (list 1 2 "abc" "\"") "def")
; (1 2 abc ") def
;=>nil

;>>> optional=True
;;
;; -------- Optional Functionality --------

;; Testing keywords
(= :abc :abc)
;=>t
(= :abc :def)
;=>nil
(= :abc ":abc")
;=>nil



;; Testing recursive tail-call function

(def! sum2 (fn* (n acc) (if (= n 0) acc (sum2 (- n 1) (+ n acc)))))

;; TODO: test let*, and do for TCO

(sum2 10 0)
;=>55

(def! res2 nil)
;=>nil
(def! res2 (sum2 10000 0))
res2
;=>50005000


;; Test mutually recursive tail-call functions

(def! foo (fn* (n) (if (= n 0) 0 (bar (- n 1)))))
(def! bar (fn* (n) (if (= n 0) 0 (foo (- n 1)))))

(foo 10000)
;=>0
;;; TODO: really a step5 test
;;
;; Testing that (do (do)) not broken by TCO
(do (do 1 2))
;=>2

;;
;; Testing read-string, eval and slurp
(read-string "(1 2 (3 4) nil)")
;=>(1 2 (3 4) nil)

(read-string "(+ 2 3)")
;=>(+ 2 3)

(read-string "7 ;; comment")
;=>7

;;; Differing output, but make sure no fatal error
(read-string ";; comment")


(eval (read-string "(+ 2 3)"))
;=>5

(slurp "../tests/test.txt")
;=>"A line of text\n"

;; Testing load-file

(load-file "../tests/inc.rd")
(inc1 7)
;=>8
(inc2 7)
;=>9
(inc3 9)
;=>12

;>>> deferrable=True
;>>> optional=True
;;
;; -------- Deferrable/Optional Functionality --------

;; Testing comments in a file
(load-file "../tests/incB.rd")
; "incB.rd finished"
;=>"incB.rd return string"
(inc4 7)
;=>11
(inc5 7)
;=>12

;; Testing map literal across multiple lines in a file
(load-file "../tests/incC.rd")
mylist
;=>("a" 1)


;; Testing cons function
(cons 1 (list))
;=>(1)
(cons 1 (list 2))
;=>(1 2)
(cons 1 (list 2 3))
;=>(1 2 3)
(cons (list 1) (list 2 3))
;=>((1) 2 3)

(def! a (list 2 3))
(cons 1 a)
;=>(1 2 3)
a
;=>(2 3)

;; Testing concat function
(concat)
;=>nil
(concat (list 1 2))
;=>(1 2)
(concat (list 1 2) (list 3 4))
;=>(1 2 3 4)
(concat (list 1 2) (list 3 4) (list 5 6))
;=>(1 2 3 4 5 6)
(concat (concat))
;=>nil
(concat (list) (list))
;=>nil

(def! a (list 1 2))
(def! b (list 3 4))
(concat a b (list 5 6))
;=>(1 2 3 4 5 6)
a
;=>(1 2)
b
;=>(3 4)

;; Testing regular quote
(quote 7)
;=>7
(quote (1 2 3))
;=>(1 2 3)
(quote (1 2 (3 4)))
;=>(1 2 (3 4))

;; Testing simple quasiquote
(quasiquote 7)
;=>7
(quasiquote (1 2 3))
;=>(1 2 3)
(quasiquote (1 2 (3 4)))
;=>(1 2 (3 4))
(quasiquote (nil))
;=>(nil)

;; Testing unquote
(quasiquote (unquote 7))
;=>7
(def! a 8)
;=>8
(quasiquote a)
;=>a
(quasiquote (unquote a))
;=>8
(quasiquote (1 a 3))
;=>(1 a 3)
(quasiquote (1 (unquote a) 3))
;=>(1 8 3)
(def! b (quote (1 "b" "d")))
;=>(1 "b" "d")
(quasiquote (1 b 3))
;=>(1 b 3)
(quasiquote (1 (unquote b) 3))
;=>(1 (1 "b" "d") 3)
(quasiquote ((unquote 1) (unquote 2)))
;=>(1 2)

;; Testing splice-unquote
(def! c (quote (1 "b" "d")))
;=>(1 "b" "d")
(quasiquote (1 c 3))
;=>(1 c 3)
(quasiquote (1 (splice-unquote c) 3))
;=>(1 1 "b" "d" 3)


;; Testing symbol equality
(= (quote abc) (quote abc))
;=>t
(= (quote abc) (quote abcd))
;=>nil
(= (quote abc) "abc")
;=>nil
(= "abc" (quote abc))
;=>nil
(= "abc" (str (quote abc)))
;=>t
(= (quote abc) nil)
;=>nil
(= nil (quote abc))
;=>nil

;;;;; Test quine
;;; TODO: needs expect line length fix
;;;((fn* [q] (quasiquote ((unquote q) (quote (unquote q))))) (quote (fn* [q] (quasiquote ((unquote q) (quote (unquote q)))))))
;;;=>((fn* [q] (quasiquote ((unquote q) (quote (unquote q))))) (quote (fn* [q] (quasiquote ((unquote q) (quote (unquote q)))))))

;>>> deferrable=True
;;
;; -------- Deferrable Functionality --------

;; Testing ' (quote) reader macro
'7
;=>7
'(1 2 3)
;=>(1 2 3)
'(1 2 (3 4))
;=>(1 2 (3 4))

;; Testing ` (quasiquote) reader macro
`7
;=>7
`(1 2 3)
;=>(1 2 3)
`(1 2 (3 4))
;=>(1 2 (3 4))
`(nil)
;=>(nil)

;; Testing ~ (unquote) reader macro
`~7
;=>7
(def! a 8)
;=>8
`(1 ~a 3)
;=>(1 8 3)
(def! b '(1 "b" "d"))
;=>(1 "b" "d")
`(1 b 3)
;=>(1 b 3)
`(1 ~b 3)
;=>(1 (1 "b" "d") 3)

;; Testing ~@ (splice-unquote) reader macro
(def! c '(1 "b" "d"))
;=>(1 "b" "d")
`(1 c 3)
;=>(1 c 3)
`(1 ~@c 3)
;=>(1 1 "b" "d" 3)


;; Testing trivial macros
(defmacro! one (fn* () 1))
(one)
;=>1
(defmacro! two (fn* () 2))
(two)
;=>2

;; Testing unless macros
(defmacro! unless (fn* (pred a b) `(if ~pred ~b ~a)))
(unless nil 7 8)
;=>7
(unless t 7 8)
;=>8
(defmacro! unless2 (fn* (pred a b) `(if (not ~pred) ~a ~b)))
(unless2 nil 7 8)
;=>7
(unless2 t 7 8)
;=>8

;; Testing macroexpand
(macroexpand (unless2 2 3 4))
;=>(if (not 2) 3 4)

;; Testing evaluation of macro result
(defmacro! identity (fn* (x) x))
(let* (a 123) (identity a))
;=>123


;>>> deferrable=True
;;
;; -------- Deferrable Functionality --------

;; Testing non-macro function
(not (= 1 1))
;=>nil
;;; This should fail if it is a macro
(not (= 1 2))
;=>t

;; Testing nth, first and rest functions

(nth (list 1) 0)
;=>1
(nth (list 1 2) 1)
;=>2
(def! x "x")
(def! x (nth (list 1 2) 2))
x
;=>"x"

(first (list))
;=>nil
(first (list 6))
;=>6
(first (list 7 8 9))
;=>7

(rest (list))
;=>nil
(rest (list 6))
;=>nil
(rest (list 7 8 9))
;=>(8 9)


;; Testing or macro
(or)
;=>nil
(or 1)
;=>1
(or 1 2 3 4)
;=>1
(or nil 2)
;=>2
(or nil nil 3)
;=>3
(or nil nil nil nil nil 4)
;=>4
(or nil nil 3 nil nil 4)
;=>3
(or (or nil 4))
;=>4

;; Testing cond macro

(cond)
;=>nil
(cond t 7)
;=>7
(cond t 7 t 8)
;=>7
(cond nil 7 t 8)
;=>8
(cond nil 7 nil 8 "else" 9)
;=>9
(cond nil 7 (= 2 2) 8 "else" 9)
;=>8
(cond nil 7 nil 8 nil 9)
;=>nil

;; Testing EVAL in let*

(let* (x (or nil "yes")) x)
;=>"yes"


;>>> optional=True
;;
;; -------- Optional Functionality --------

;;
;; Loading core.rd
(load-file "../tests/core.rd")

;; Testing -> macro
(-> 7)
;=>7
(-> (list 7 8 9) first)
;=>7
(-> (list 7 8 9) (first))
;=>7
(-> (list 7 8 9) first (+ 7))
;=>14
(-> (list 7 8 9) rest (rest) first (+ 7))
;=>16

;; Testing ->> macro
(->> "L")
;=>"L"
(->> "L" (str "A") (str "M"))
;=>"MAL"
(->> (4) (concat (3)) (concat (2)) rest (concat (1)))
;=>(1 3 4)

