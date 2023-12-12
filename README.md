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
  <li>You will need them all in one directory, on a system that is able to run C with the following libraries:
  <ul>
    <li>curses.h</li>
    <li>stdio.h</li>
    <li>stdlib.h</li>
    <li>unistd.h</li>
    <li>errno.h</li>
    <li>fcntl.h</li>
    <li>sys/stat.h</li>
    <li>stdbool.h</li>
    <li>pthread.h</li>
    <li>string.h</li>
  </ul>
    <li>These libraries do not need to be preinstalled, you just need a version of C that can run them</li>
  </li>
</ul>
        
### how to interact
<ul>
  <li>To compile this program, while in the directory with the aforementioned files, type <code>make main</code> in your terminal.</li>
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

### how to read the UI
<ul>
  <li>The top line of the UI will have the username, whether the user is a host or client, and if host, the port to connect to</li>
  <li>The second line will be where to type, and there will be a prompt for what is expected to be there
    <ul>
      <li>If the prompt says Line num: put in a line number 1-40, matching the line of the file you want to edit, or :q to quit</li>
      <li>If the prompt says Contents: put in what you want written at the line you previously specified</li>
    </ul>
  </li>
  <li>The next 40 lines represent what is currently stored at each line of the file</li>
  <li>Each of these 40 lines has either an asterisk or space in front of the line number
  <ul>
    <li>An asterisk indicates that the line is in use by at least one other user</li>
    <li>A space indicates the line is not currently in use by any user</li>
  </ul>
  </li>
  <li>If the host quits, the next time a client gives input, they will recieve a screen saying the host has terminated the session and they no longer have file access
    <ul>
      <li>This message will persist for 5 seconds, after which the client program will also terminate</li>
    </ul>
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

### line content restrictions
<ul>
  <li>Like the original file lines, users cannot write content longer than 100 characters to lines, the program stops reading characters past 100</li>
  <li>Line contents submitted by users are not allowed to be a " " (a single space and nothing else) or "*" (a single asterisk and nothing else)
    <ul>
      <li>This is because these messages are used to denote marking a given line as used or unused, and there is no way to distinguish the two possible use cases</li>
      <li>If either of these messages are needed for visual effect, we recommend "* " (asterisk+space) in place of asterisk and two spaces in place of space</li>
    </ul>
  </li>
</ul>

## example usage
<ol>
  <li>Amani wants to host an editing session for her lab, <code>some213.c</code>. She is on the computer wilkinson.grinnell.cs.edu</li> <li>She runs the program using the command line arguments, <code>./main amani /home/csc213/labs/some213.c</code>, which is a correct filepath to her <code>some213.c</code> lab
  <ol>
    <li><code>some213.c</code> is a pretty short file, 40 lines long exactly, and none of the lines are over 100 characters</li>
    <li>The program renders all 40 lines of <code>some213.c</code> with their appropriate line numbers</li>
    <li>Above the rendering of the line numbers, there's a username line that says the port number to connect to Amani with. Let's call it 59846</li>
  </ol>
  </li>
  <li>Amani wants to edit line 37 of the file, so she types 37 on the line number prompt
    <ol>
      <li>An asterisk appears next to line 37, indicating that the line is use by someone.
        <ul> <li>And it is! It is in use by Amani!</li></ul>
      </li>
    </ol>
  </li>
  <li>While Amani is typing her content in the Contents prompt, one of Amani's lab partners, Istar, joins the session, by using the command line arguments <code>./main istar wilkinson.grinnell.cs.edu 59846</code> 
    <ol><li>Istar renders the same code Amani has, and the asterisk already appears at line 37</li></ol>
  </li>
  <li>Istar claims line 15, and an asterisk appears there too, on both Istar and Amani's screen</li>
  <li>Amani types her contents for line 37, a comment that says "//what", and then that is loaded to line 37 on everyone's screen, and the file itself.
  <ol>
    <li>The asterisk is replaced by a space, to denote that the line is no longer in use.</li>
  </ol>
  </li>
  <li>Istar types her contents for line 15, it's renaming an int pointer from "ptr" to "int_ptr"
    <ol>
      <li>This change is printed to everyone's screens, and through the host, gets written to the file</li>
      <li>The asterisk also changes to a space, seeing as the line is no longer in use</li>
    </ol>
  </li>
  <li>The third lab partner, Ellie, now also joins the editing session, using the command line arguments <code>./main ellie wilkinson.grinnell.cs.edu 59846</code>
    <ol>
      <li>It renders all the contents, including the changes Istar and Amani have made </li>
      <li>At this point, no lines are claimed, so there are all spaces and no asterisks to the left of all the line numbers</li>
    </ol>
  </li>
  <li>More miscellaneous changes are made as the group works on the lab. Imagine a groupwork montage here. Now, skip ahead in time to a moment where Ellie has line 6 claimed, Amani has line 5 claimed, and Istar has nothing claimed. There's a big bug on line 7 in the file, so they ask Prof. Perlmutter for help with their lab, in a kind of office hours sort of scenario. Professor Perlmutter joins the editing session with the command line <code>./main leah wilkinson.grinnell.cs.edu 59846</code></li>
  <li>Oh no! The troubles of sychronous editing have occured. In an attempt to fix the bug, people all try to claim line 7. 
    <ol>
      <li>In order, Ellie tries to claim it
        <ul><li>There is now an asterisk beside line 7 on everyone's screen</li></ul>
      </li>
      <li>Then Istar tries to claim it
        <ul><li>The asterisk looks the same as before, it indicates if there's someone there, not how many people</li></ul>
      </li>
      <li>Then Leah tries to claim it
        <ul><li>The asterisk looks the same as before, it indicates if there's someone there, not how many people</li></ul>
      </li>
        <li>Then Amani tries to claim it
        <ul><li>The asterisk looks the same as before, it indicates if there's someone there, not how many people</li></ul>
      </li>
    </ol>
  </li>
<li>Now having all entered (and marked as using) line 7, they push the changes that try to fix the bug.
  <ol>
    <li>Istar pushes her changes first
      <ul><li>Her changes show up on everyone's screen</li></ul>
      <ul><li>There is still an asterisk next to line 7 because the line is still in use by others</li></ul>
    </li>
    <li>Ellie pushes their changes next
      <ul><li>Their changes show up on everyone's screen, overwriting those Istar just did. Sorry Istar!</li></ul>
      <ul><li>There is still an asterisk next to line 7 because the line is still in use by others</li></ul>
    </li>
    <li>Prof Perlmutter pushes her changes next
      <ul><li>Her changes show up on everyone's screen, overwriting those Ellie just did. Sorry Ellie!</li></ul>
      <ul><li>There is still an asterisk next to line 7 because the line is still in use by others</li></ul>
    </li>
      <li>Amani pushes her changes next
      <ul><li>Her changes show up on everyone's screen, overwriting those Prof Perlmutter just did. Sorry Professor Perlmutter!</li></ul>
      <ul><li>There is no longer an asterisk next to line 7 because the line isn't in use by anyone anymore</li></ul>
    </li>
  </ol>
</li>
<li>Somehow, throughout all of this chaos, the bug did actually get fixed, so Prof Perlmutter doesn't need to be here anymore.
  <ol>
  <li>By typing :q where the line number would have otherwise gone, she is able to quit the program</li>
  <li>It clears her screen and exits curses mode, so she can use her terminal normally again</li>
  <li>Because she is a client, not the host, this does not impact the other users at all</li>
  </ol>
</li>
  
</li>Amani has a meeting to go to, so she needs to leave too.
  <li>By typing :q where the line number would have otherwise gone, she is able to quit the program</li>
  <li>It clears her screen and exits curses mode, so she can use her terminal normally again</li>
  <li>Because she is the host, this has an impact on the two remaining clients, Istar and Ellie.
    <ul><li>The next time Ellie or Istar give an input, if it is content, it won't write anywhere, and the next time they input a line number, instead of claiming that line, their window will be cleared and replaced with a message saying that the host has ended the session and they no longet have document access
      <ul>
      <li>This message will hang out for five seconds and then be dismissed, leaving their terminals free for normal use.</li>
      <li>They will each experience this behavior separately, based on their own inputs, not each others'.</li>
      </ul>
    </li> </ul>
  </li>
  </li>
  </ol>
</ol>


