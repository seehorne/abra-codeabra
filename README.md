# collaborative-code-editor
Grinnell College CSC-213 Fall '23 Final Project
## description
This project is a collaborative code editor that can be run on Mathlan.
## general use
### how to run
<ul>
  <li>To compile this program, type <code>make main</code> in your terminal.</li>
  <li>To run this program use one of the following two command line prompts
  <ol>
    <li>If you are launching <strong>new</strong>  editing session: <code>./main &ltusername&gt &ltfilepath&gt</code> </li>
      <ul> <li> If you are starting the editing session but are not the owner of the file to be edited, please ensure you have, at minimum, rw permissions to the file at your filepath before running</li></ul>
    <li>If you are joining an <strong>existing</strong> editing session: <code>./main &ltusername&gt &lthostname&gt &ltsocket to connect to&gt</code></li>
      <ul>
        <li> Hostname refers to the machine the session host is using, not the host's username</li>
        <li> Contact your session host to determine this machine and the session socket</li>
      </ul>
  </ol></li>
</ul>

### file requirements
<p> The collaborative code editor supports editing for documents with at most 40 lines, with lines of at most 100 characters each. Please take care to ensure files you edit with collaborative-code-editor meet these requirements. <strong>Any lines over 40 will be truncated from the file, as will any line contents past 100 characters.</strong></p>
