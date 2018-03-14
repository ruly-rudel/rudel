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
;=>()

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
;=>()
(list? (list))
;=>true
(empty? (list))
;=>true
(empty? (list 1))
;=>false
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
(if true 7 8)
;=>7
(if false 7 8)
;=>8
(if true (+ 1 7) (+ 1 8))
;=>8
(if false (+ 1 7) (+ 1 8))
;=>9
(if nil 7 8)
;=>8
(if 0 7 8)
;=>7
(if "" 7 8)
;=>7
(if (list) 7 8)
;=>7
(if (list 1 2 3) 7 8)
;=>7
(= (list) nil)
;=>false


;; Testing 1-way if form
(if false (+ 1 7))
;=>nil
(if nil 8 7)
;=>7
(if true (+ 1 7))
;=>8


;; Testing basic conditionals
(= 2 1)
;=>false
(= 1 1)
;=>true
(= 1 2)
;=>false
(= 1 (+ 1 1))
;=>false
(= 2 (+ 1 1))
;=>true
(= nil 1)
;=>false
(= nil nil)
;=>true

(> 2 1)
;=>true
(> 1 1)
;=>false
(> 1 2)
;=>false

(>= 2 1)
;=>true
(>= 1 1)
;=>true
(>= 1 2)
;=>false

(< 2 1)
;=>false
(< 1 1)
;=>false
(< 1 2)
;=>true

(<= 2 1)
;=>false
(<= 1 1)
;=>true
(<= 1 2)
;=>true


;; Testing equality
(= 1 1)
;=>true
(= 0 0)
;=>true
(= 1 0)
;=>false
(= "" "")
;=>true
(= "abc" "abc")
;=>true
(= "abc" "")
;=>false
(= "" "abc")
;=>false
(= "abc" "def")
;=>false
(= "abc" "ABC")
;=>false
(= true true)
;=>true
(= false false)
;=>true
(= nil nil)
;=>true

(= (list) (list))
;=>true
(= (list 1 2) (list 1 2))
;=>true
(= (list 1) (list))
;=>false
(= (list) (list 1))
;=>false
(= 0 (list))
;=>false
(= (list) 0)
;=>false
(= (list) "")
;=>false
(= "" (list))
;=>false


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
;=>true
( (fn* (& more) (count more)) 1)
;=>1
( (fn* (& more) (count more)) )
;=>0
( (fn* (& more) (list? more)) )
;=>true
( (fn* (a & more) (count more)) 1 2 3)
;=>2
( (fn* (a & more) (count more)) 1)
;=>0
( (fn* (a & more) (list? more)) 1)
;=>true


;; Testing language defined not function
(not false)
;=>true
(not true)
;=>false
(not "a")
;=>false
(not 0)
;=>false


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
;=>"()"

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
;=>"()"

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
;=>true
(= :abc :def)
;=>false
(= :abc ":abc")
;=>false



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
;=>()
(concat (list 1 2))
;=>(1 2)
(concat (list 1 2) (list 3 4))
;=>(1 2 3 4)
(concat (list 1 2) (list 3 4) (list 5 6))
;=>(1 2 3 4 5 6)
(concat (concat))
;=>()
(concat (list) (list))
;=>()

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
;=>true
(= (quote abc) (quote abcd))
;=>false
(= (quote abc) "abc")
;=>false
(= "abc" (quote abc))
;=>false
(= "abc" (str (quote abc)))
;=>true
(= (quote abc) nil)
;=>false
(= nil (quote abc))
;=>false

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
(unless false 7 8)
;=>7
(unless true 7 8)
;=>8
(defmacro! unless2 (fn* (pred a b) `(if (not ~pred) ~a ~b)))
(unless2 false 7 8)
;=>7
(unless2 true 7 8)
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
;=>false
;;; This should fail if it is a macro
(not (= 1 2))
;=>true

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
;=>()
(rest (list 6))
;=>()
(rest (list 7 8 9))
;=>(8 9)


;; Testing or macro
(or)
;=>nil
(or 1)
;=>1
(or 1 2 3 4)
;=>1
(or false 2)
;=>2
(or false nil 3)
;=>3
(or false nil false false nil 4)
;=>4
(or false nil 3 false nil 4)
;=>3
(or (or false 4))
;=>4

;; Testing cond macro

(cond)
;=>nil
(cond true 7)
;=>7
(cond true 7 true 8)
;=>7
(cond false 7 true 8)
;=>8
(cond false 7 false 8 "else" 9)
;=>9
(cond false 7 (= 2 2) 8 "else" 9)
;=>8
(cond false 7 false 8 false 9)
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
(->> [4] (concat [3]) (concat [2]) rest (concat [1]))
;=>(1 3 4)

