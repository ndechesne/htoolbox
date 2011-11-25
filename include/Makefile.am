htoolsincludedir = $(includedir)/htoolbox

AUTO_GENERATED = report_macros.h

all-local: $(AUTO_GENERATED)

clean-local:
	rm -f $(AUTO_GENERATED)

report_macros.h: create_report_macros.sh
	./$< > $@

htoolsinclude_HEADERS = \
  asyncwriter.h \
  configuration.h \
  copier.h \
  criticality.h \
  filereaderwriter.h \
  files.h \
  filesystem.h \
  hash_tree.h \
  hasher.h \
  ireaderwriter.h \
  linereaderwriter.h \
  multiwriter.h \
  nullwriter.h \
  observer.h \
  process_mutex.h \
  queue.h \
  report.h \
  report_macros.h \
  share.h \
  shared_path.h \
  socket.h \
  stackhelper.h \
  tlv.h \
  tlv_helper.h \
  threads_manager.h \
  zipper.h \
  $(NULL)

EXTRA_DIST = \
  asyncwriter.h \
  configuration.h \
  copier.h \
  criticality.h \
  filereaderwriter.h \
  files.h \
  filesystem.h \
  hash_tree.h \
  hasher.h \
  ireaderwriter.h \
  linereaderwriter.h \
  multiwriter.h \
  nullwriter.h \
  observer.h \
  process_mutex.h \
  queue.h \
  report.h \
  share.h \
  shared_path.h \
  socket.h \
  stackhelper.h \
  tlv.h \
  tlv_helper.h \
  threads_manager.h \
  zipper.h \
  $(NULL)
