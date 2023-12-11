# abra codeabra
Grinnell College CSC-213 Fall '23 Final Project
## description
This project is a collaborative code editor that can be run on Mathlan.
## general use
### how to set up
<ul>
  <li>
    In order to run this program, you will need the following .c and .h files (all listed on this Github)
    <ul>
      <li>main.c: the main program</li>
      <li>message.c: file to send and recieve messages, adapted from p2p lab</li>
      <li>message.h: header file for message.c, from p2p lab</li>
      <li>socket.h: header file for distributed systems, from p2p lab</li>
      <li>Makefile: file to help compile and run the project</li>
    </ul>
  </li>
  <li>These files can be downloaded from this <a href=https://github.com/seehorne/abra-codeabra> Github repo</a> </li>
</ul>
        
### how to interact
<ul>
  <li>To compile this program, type <code>make main</code> in your terminal.</li>
  <li>To run this program, use one of the following two command line prompts:
  <ol>
    <li>To launch a <strong>new</strong> editing session: <code>./main &ltusername&gt &ltfilepath&gt</code> </li>
      <ul> <li> If you are starting the editing session you will be the owner of the file. Clients connecting can edit and view a representation of the file, but once the session ends the real file is only saved to the host's directory.</li></ul>
    <li>To join an <strong>existing</strong> editing session: <code>./main &ltusername&gt &lthostname&gt &ltsocket to connect to&gt</code></li>
      <ul>
        <li> hostname refers to the machine the session host is using, not the host's username</li>
        <li> Contact your session host to determine this machine and the session socket</li>
      </ul>
  </ol></li>
  <li> To leave this program, type :q where you would normally type the line number
    <ul><li>If you are the session host and you leave, everyone gets kicked out.</li>
      <li>If you are not the session host, you leaving does not impact the session.</li></ul>
  </li>
</ul>

### file requirements
<ul>
<li> The collaborative code editor supports the editing of documents with at most 40 lines, with lines of at most 100 characters each. Please take care to ensure files you edit with collaborative-code-editor meet these requirements.
  <ul><li> <strong>Any lines over 40 will be truncated from the file, as will any line contents past 100 characters.</strong></li>
  <li>Additionally, code with lines shorter that 100 characters will have lines padded with spaces up to 100 characters, and code with under 40 lines will have lines of spaces added up to 40 lines. If this kind of whitespace impacts the code you write, please be aware of this behavior.</li></ul>
</li>
  <li>Files are required to be able to be representable as plaintext (mainly code files and .txt files). Behavior for files that do not meet this requirement, such as PDFs, jpgs, mp3s, and executables is not defined and will not be handled.</li>
  <li>Files are not required to exist before the program runs. 
    <ul> <li>If the filepath provided does not lead to a real file, the file will be created in that location, and a blank file will be loaded in the editor (blank as in 40 lines of spaces), so long as the session host has permission to make said file at that location. 
    <ul> <li> This program does not handle the session host trying to create a file in a directory where they do not have permission to do so. </li></ul></li></ul>
  </li>
</ul>


