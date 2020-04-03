(* FASTA Parsing *)

let is_label str =( String.length str > 1) && str.[0] = '>' ;;

let get_label str = String.sub str 1 (String.length str - 1) ;;

let read_fasta chan =
  let res = ref [] in
  let cont = Buffer.create 80 in
  let rec doloop currlabel line =
    if is_label line then begin
      (* If it's not the first string in the file, add previous string to the list *)
      if currlabel <> "" then res := ((currlabel, Buffer.contents cont) :: !res) ;
      Buffer.clear cont; doloop (get_label line) (input_line chan)
    end
    else begin
      Buffer.add_string cont line;
      try doloop currlabel (input_line chan)
      with End_of_file ->
        res := ((currlabel, Buffer.contents cont) :: !res); List.rev !res
    end
  in try match input_line chan with
    | line when is_label line -> doloop "" line
    | _ -> failwith "Badly formatted FASTA file?"
  with End_of_file -> List.rev !res ;;

(* ------------------------------------------------------------------- *)

(* Utilities *)
let fst (a,_) = a ;;
let snd (_,b) = b ;;

let list_of_string str =
  let result = ref [] in
  String.iter (fun x -> result := x :: !result)
              str;
  List.rev !result

let string_of_list lst =
  let result = Bytes.create (List.length lst) in
  ignore (List.fold_left (fun i x -> Bytes.set result i x; i+1) 0 lst);
  Bytes.to_string result

let fstl l = List.map fst l
let sndl l = List.map snd l

(** Helper function to unzip a list of couples *)
let unzip l = if l = [] then ([], []) else
    (fstl l, sndl l)

(* Zip together two lists with in single list of couples *)
let rec zip l1 l2 =
  match (l1, l2) with
  | ([], []) -> []
  | (x::xs, y::ys) -> (x,y)::(zip xs ys)
  | _ -> failwith "lists are not of equal length"


let rec pow a = function
  | 0 -> 1
  | 1 -> a
  | n ->
    let b = pow a (n / 2) in
    b * b * (if n mod 2 = 0 then 1 else a)

let reverse_string s =
  let len = String.length s in
  String.init len (fun i -> s.[len - 1 - i ])


(* ------------------------------------------------------------------- *)

(* RNA conversion *)

let rna_of_dna s = String.map (fun c -> match c with
  | 'T' -> 'U'
  | a -> a ) s

let trim_string_to_mod modulo string =
  let len = String.length string in
  String.sub string 0 (len - (len mod modulo))

(* RNA Codon table *) 

(* The 20 commonly occurring amino acids are abbreviated by using 20 letters
from the English alphabet (all letters except for B, J, O, U, X, and Z). Protein
strings are constructed from these 20 symbols. Henceforth, the term genetic
string will incorporate protein strings along with DNA strings and RNA strings.
The RNA codon table dictates the details regarding the encoding of specific
codons into the amino acid alphabet. *)

let encode_codon_protein codon =
  if String.length codon <> 3 then failwith @@ codon ^ " is not a valid codon" else
  match codon with
  | "AAA" -> 'K' | "AAC" -> 'N' | "AAG" -> 'K' | "AAU" -> 'N'
  | "ACA" -> 'T' | "ACC" -> 'T' | "ACG" -> 'T' | "ACU" -> 'T'
  | "AGA" -> 'R' | "AGC" -> 'S' | "AGG" -> 'R' | "AGU" -> 'S'
  | "AUA" -> 'I' | "AUC" -> 'I' | "AUG" -> 'M' | "AUU" -> 'I'
  | "CAA" -> 'Q' | "CAC" -> 'H' | "CAG" -> 'Q' | "CAU" -> 'H'
  | "CCA" -> 'P' | "CCC" -> 'P' | "CCG" -> 'P' | "CCU" -> 'P'
  | "CGA" -> 'R' | "CGC" -> 'R' | "CGG" -> 'R' | "CGU" -> 'R'
  | "CUA" -> 'L' | "CUC" -> 'L' | "CUG" -> 'L' | "CUU" -> 'L'
  | "GAA" -> 'E' | "GAC" -> 'D' | "GAG" -> 'E' | "GAU" -> 'D'
  | "GCA" -> 'A' | "GCC" -> 'A' | "GCG" -> 'A' | "GCU" -> 'A'
  | "GGA" -> 'G' | "GGC" -> 'G' | "GGG" -> 'G' | "GGU" -> 'G'
  | "GUA" -> 'V' | "GUC" -> 'V' | "GUG" -> 'V' | "GUU" -> 'V'
  | "UAC" -> 'Y' | "UAU" -> 'Y' | "UCA" -> 'S' | "UCC" -> 'S'
  | "UCG" -> 'S' | "UCU" -> 'S' | "UGC" -> 'C' | "UGG" -> 'W'
  | "UGU" -> 'C' | "UUA" -> 'L' | "UUC" -> 'F' | "UUG" -> 'L'
  | "UUU" -> 'F' | "UAG" -> '0' | "UGA" -> '0' | "UAA" -> '0'
  | _ -> failwith "invalid codon" ;;

let protein_from_mrna rna =
  let rna_len = String.length rna in
  let protein_len = rna_len / 3 in
  if rna_len mod 3 <> 0 then
    failwith "mRNA string is not a sequence of codons"
  else
  let buf = Buffer.create protein_len in
  let rec encode rna =
    match rna with
      | "" -> Buffer.contents buf
      | _ ->
        let codon = String.sub rna 0 3 in
        let protein = encode_codon_protein codon in
        Buffer.add_char buf protein;
        encode (String.sub rna 3 (String.length rna - 3)) in
  encode rna

(* ------------------------------------------------------------------- *)


(* Map each amminoacid to an integer *)
let int_of_amminoacid = function
  | 'A' -> 0
  | 'C' -> 1
  | 'D' -> 2
  | 'E' -> 3
  | 'F' -> 4
  | 'G' -> 5
  | 'H' -> 6
  | 'I' -> 7
  | 'K' -> 8
  | 'L' -> 9
  | 'M' -> 10
  | 'N' -> 11
  | 'P' -> 12
  | 'Q' -> 13
  | 'R' -> 14
  | 'S' -> 15
  | 'T' -> 16
  | 'V' -> 17
  | 'W' -> 18
  | 'Y' -> 19
  | '0' -> 20
  | _ -> failwith "not amminoacid"

(** Get a list of integers from an amminoacid string  *)
let list_of_amminoacid string = List.map int_of_amminoacid @@ list_of_string string

(**  Get an octave from an amminoacid integer*)
let octave_of_int num = num mod 8

(** Get a delay from the start of the bar, in 16th *)
let delay_of_int num = (num mod 16) * 60

(** Get a note duration, in 16th *)
let duration_of_int num = ((num mod 16) + 1) * 60

(** Get a note number 0-11 *)
let note_of_int num = num mod 12

(** Get a note number 0-7 *)
let note_in_scale_of_int num = num mod 7

(* note in major scale to chromatic conversion *)
let autoscale_major offset note =
  offset + (match note with
    | 0 -> offset
    | 1 -> offset + 2
    | 2 -> offset + 4
    | 3 -> offset + 5
    | 4 -> offset + 7
    | 5 -> offset + 9
    | 6 -> offset + 11
    | 7 -> offset + 12
    | _ -> failwith "not a note in scale")

(* note in minor scale to chromatic conversion *)
let autoscale_minor offset note =
  offset + (match note with
    | 0 -> offset
    | 1 -> offset + 2
    | 2 -> offset + 3
    | 3 -> offset + 5
    | 4 -> offset + 7
    | 5 -> offset + 8
    | 6 -> offset + 10
    | 7 -> offset + 12
    | _ -> failwith "not a note in scale")


(** String-ify a note in txt2mid format *)
let string_of_note ~bar ~date ~dur ~oct ~note =
  Printf.sprintf "N %d %d %d %d %d %d\n"
    ((bar * 960) + date) oct note 100 dur 1

(** Magic: read amminoacid integers 4 at a time, in sequences of
 notes_in_bar notes. The first sector in a sequence encodes how
 many notes will be in that bar. *)
let rec encode ~buf ~seq ~bar ~notes_in_bar ~current_note_in_bar
  ?minor:(minor=false) ?scale:(scale=0) () =
  match seq with
  | loc :: dur :: oct :: note :: xs ->
(*     Printf.printf "%d %d\n" notes_in_bar current_note_in_bar;
 *)    if current_note_in_bar = -1 then
      (* Use the first symbol of the seq to encode how
        many notes will fit in that bar *)
      encode
        ~buf:buf
        ~seq:(dur :: oct :: note :: xs)
        ~bar:bar
        ~notes_in_bar:((loc mod 16) + 1)
        ~current_note_in_bar:0
        ~scale:scale ~minor:minor
        ()
    else if current_note_in_bar >= notes_in_bar then begin

      (* Reset the bar counter *)
      encode
        ~buf:buf
        ~seq:seq
        ~bar:(bar + 1)
        ~notes_in_bar:notes_in_bar
        ~current_note_in_bar:(-1)
        ~scale:scale ~minor:minor
        () end
    else
      let note_string =
        string_of_note
          ~bar:bar
          ~date:(delay_of_int loc)
          ~dur:(duration_of_int dur)
          ~oct:(octave_of_int oct)
          ~note:(
            if scale >= 0 then
              if minor then
                autoscale_minor scale (note_in_scale_of_int note)
              else
                autoscale_minor scale (note_in_scale_of_int note)
            else note_of_int note) in
      Buffer.add_string buf note_string;
      encode
        ~buf:buf
        ~seq:xs
        ~bar:bar
        ~notes_in_bar:notes_in_bar
        ~current_note_in_bar:(current_note_in_bar + 1)
        ~scale:scale ~minor:minor
        ()
  | _ -> ()


(* ------------------------------------------------------------------- *)


(* Main *)

let () =
  (* Read, merge and trim dna strings from a FASTA file *)
  let str = String.concat "" @@ sndl @@ read_fasta stdin in
  let seq = list_of_amminoacid @@ protein_from_mrna @@ rna_of_dna @@ trim_string_to_mod 3 str in
  let buf = Buffer.create (List.length seq * 8) in
  let argc = Array.length Sys.argv in
  if argc >= 3 then failwith "Could not parse arguments";
  let scale = if argc > 1 then int_of_string @@ Sys.argv.(1) else -1 in
  let minor = if argc > 2 then bool_of_string @@ Sys.argv.(2) else false in
  encode
    ~buf:buf
    ~seq:seq
    ~bar:0
    ~notes_in_bar:0
    ~current_note_in_bar:(-1)
    ~scale:scale ~minor:minor
    ();
  print_endline @@ Buffer.contents buf
