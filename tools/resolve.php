<?php
/*
resolve.php v2.0
Created 29.07.2011, Bremen, Germany

Copyright (c) 2011 Christian F. Coors (Cuervo, member of the PrettyOS-team)

Christian F. Coors
Waiblinger Weg 37B
28215 Bremen

http://www.fanofblitzbasic.de/
All rights reserved.


BITTE LESEN SIE DIESEN SOFTWARELIZENZVERTRAG („LIZENZ“) SORGFÄLTIG DURCH,
BEVOR SIE DIE SOFTWARE IN BETRIEB NEHMEN. INDEM SIE DIE SOFTWARE VERWENDEN,
ERKLÄREN SIE IHR EINVERSTÄNDNIS MIT DEN BESTIMMUNGEN DES NACHSTEHENDEN
LIZENZVERTRAGS. WENN SIE MIT DEN BESTIMMUNGEN DIESES LIZENZVERTRAGS NICHT
EINVERSTANDEN SIND, INSTALLIEREN UND/ODER VERWENDEN SIE DIE SOFTWARE NICHT.

Christian F. Coors erteilt Ihnen hiermit das Recht zur Benutzung
der beigefügten Software (mit Boot-ROM-Code) einschließlich
Drittanbietersoftware, Dokumentation, Benutzeroberflächen, Inhalte,
Zeichensätze und dieser Lizenz zugehörigen Daten (im folgenden „Software“),
unabhängig davon, ob diese auf Hardware vorinstalliert, auf einer CD/DVD,
einem ROM oder in anderer Form gespeichert ist.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Sie erklären sich damit einverstanden, die Software und die Dienste
in Übereinstimmung mit allen anwendbaren Gesetzen zu verwenden, einschließlich
lokale Gesetze des Landes oder der Region, in dem bzw. der Sie wohnhaft sind
oder in dem bzw. der Sie die Software und Dienste laden oder verwenden. 

Salvatorische Klausel:
Sollten einzelne Bestimmungen dieses Vertrages unwirksam oder undurchführbar
sein oder nach Vertragsschluss unwirksam oder undurchführbar werden, bleibt
davon die Wirksamkeit des Vertrages im Übrigen unberührt. An die Stelle der
unwirksamen oder undurchführbaren Bestimmung soll diejenige wirksame und
durchführbare Regelung treten, deren Wirkungen der wirtschaftlichen Zielsetzung
am nächsten kommen, die die Vertragsparteien mit der unwirksamen bzw.
undurchführbaren Bestimmung verfolgt haben. Die vorstehenden Bestimmungen
gelten entsprechend für den Fall, dass sich der Vertrag als lückenhaft erweist.

Christian F. Coors hat das Recht, diese Bedingungen jederzeit zu ändern.
Sie erklären sich damit einverstanden, auch an eventuelle neue Bedingungen
gebunden zu sein, sobald Sie von einer solchen Änderung in Kenntnis gesetzt
worden sind. Diese Vereinbarung unterliegt deutschem Recht.

Gerichtsstand ist Bremen.
*/


$name=$_GET["hostname"]; // Get resolve.php?hostname= ...
$name=htmlentities($name); // Protect against HTML-Code

$result = gethostbyname($name); // Resolve IP Address
echo $result; // Output IP Address
?>