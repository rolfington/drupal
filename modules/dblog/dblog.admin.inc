<?php
// $Id: dblog.admin.inc,v 1.16 2009-04-26 19:44:38 dries Exp $

/**
 * @file
 * Administrative page callbacks for the dblog module.
 */

/**
 * Implementation of hook_form_FORM_ID_alter().
 */
function dblog_form_system_logging_settings_alter(&$form, $form_state) {
  $form['dblog_row_limit'] = array(
    '#type' => 'select',
    '#title' => t('Database log entries to keep'),
    '#default_value' => variable_get('dblog_row_limit', 1000),
    '#options' => drupal_map_assoc(array(100, 1000, 10000, 100000, 1000000)),
    '#description' => t('The maximum number of entries to keep in the database log. Requires a <a href="@cron">cron maintenance task</a>.', array('@cron' => url('admin/reports/status')))
  );
  $form['buttons']['#weight'] = 1;
}

/**
 * Menu callback; displays a listing of log messages.
 */
function dblog_overview() {
  $filter = dblog_build_filter_query();
  $rows = array();
  $icons = array(
    WATCHDOG_DEBUG    => '',
    WATCHDOG_INFO     => '',
    WATCHDOG_NOTICE   => '',
    WATCHDOG_WARNING  => theme('image', 'misc/watchdog-warning.png', t('warning'), t('warning')),
    WATCHDOG_ERROR    => theme('image', 'misc/watchdog-error.png', t('error'), t('error')),
    WATCHDOG_CRITICAL => theme('image', 'misc/watchdog-error.png', t('critical'), t('critical')),
    WATCHDOG_ALERT    => theme('image', 'misc/watchdog-error.png', t('alert'), t('alert')),
    WATCHDOG_EMERG    => theme('image', 'misc/watchdog-error.png', t('emergency'), t('emergency')),
  );
  $classes = array(
    WATCHDOG_DEBUG    => 'dblog-debug',
    WATCHDOG_INFO     => 'dblog-info',
    WATCHDOG_NOTICE   => 'dblog-notice',
    WATCHDOG_WARNING  => 'dblog-warning',
    WATCHDOG_ERROR    => 'dblog-error',
    WATCHDOG_CRITICAL => 'dblog-critical',
    WATCHDOG_ALERT    => 'dblog-alert',
    WATCHDOG_EMERG    => 'dblog-emerg',
  );

  $output = drupal_get_form('dblog_filter_form');
  $output .= drupal_get_form('dblog_clear_log_form');

  $header = array(
    ' ',
    array('data' => t('Type'), 'field' => 'w.type'),
    array('data' => t('Date'), 'field' => 'w.wid', 'sort' => 'desc'),
    t('Message'),
    array('data' => t('User'), 'field' => 'u.name'),
    array('data' => t('Operations')),
  );

  $query = db_select('watchdog', 'w');
  $query->join('users', 'u', 'w.uid = u.uid');
  $query
    ->fields('w', array('wid', 'uid', 'severity', 'type', 'timestamp', 'message', 'variables', 'link'))
    ->addField('u', 'name');
  if (!empty($filter['where'])) {
    //setHeader may not be chainable see Line 138
    $query
      ->where($filter['where'], $filter['args'])
      ->extend('PagerDefault')->extend('TableSort')
      ->limit(50, 0)
      ->setHeader($header);
    $result = $query->execute();
  }
  else {
    //setHeader may not be chainable see Line 138
    $query
      ->extend('PagerDefault')->extend('TableSort')
      ->limit(50)
      ->setHeader($header);
    $result = $query->execute();
  }

  foreach ($result as $dblog) {
    $rows[] = array('data' =>
      array(
        // Cells
        $icons[$dblog->severity],
        t($dblog->type),
        format_date($dblog->timestamp, 'small'),
        l(truncate_utf8(_dblog_format_message($dblog), 56, TRUE, TRUE), 'admin/reports/event/' . $dblog->wid, array('html' => TRUE)),
        theme('username', $dblog),
        $dblog->link,
      ),
      // Attributes for tr
      'class' => "dblog-" . preg_replace('/[^a-z]/i', '-', $dblog->type) . ' ' . $classes[$dblog->severity]
    );
  }

  if (!$rows) {
    $rows[] = array(array('data' => t('No log messages available.'), 'colspan' => 6));
  }

  $output .= theme('table', $header, $rows, array('id' => 'admin-dblog'));
  $output .= theme('pager', NULL);

  return $output;
}

/**
 * Menu callback; generic function to display a page of the most frequent
 * dblog events of a specified type.
 */
function dblog_top($type) {

  $header = array(
    array('data' => t('Count'), 'field' => 'count', 'sort' => 'desc'),
    array('data' => t('Message'), 'field' => 'message')
  );
  $count_query = db_select('watchdog');
  $count_query->addExpression('COUNT(DISTINCT(message))');
  $count_query->condition('type', $type);

  $query = db_select('watchdog', 'w');
  $query->addExpression('COUNT(wid)', 'count');
  $query = $query
    ->fields('w', array('message', 'variables'))
    ->condition('w.type', $type)
    ->groupBy('message')
    ->groupBy('variables')
    ->extend('PagerDefault')->extend('TableSort')
    ->limit(30);
  $query = $query->setHeader($header);
  $query->setCountQuery($count_query);
  $result = $query->execute();

  $rows = array();
  foreach ($result as $dblog) {
    $rows[] = array($dblog->count, truncate_utf8(_dblog_format_message($dblog), 56, TRUE, TRUE));
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => t('No log messages available.'), 'colspan' => 2));
  }

  $output  = theme('table', $header, $rows);
  $output .= theme('pager', NULL);

  return $output;
}

/**
 * Menu callback; displays details about a log message.
 */
function dblog_event($id) {
  $severity = watchdog_severity_levels();
  $output = '';
  $result = db_query('SELECT w.*, u.name, u.uid FROM {watchdog} w INNER JOIN {users} u ON w.uid = u.uid WHERE w.wid = :id', array(':id' => $id))->fetchObject();
  if ($dblog = $result) {
    $rows = array(
      array(
        array('data' => t('Type'), 'header' => TRUE),
        t($dblog->type),
      ),
      array(
        array('data' => t('Date'), 'header' => TRUE),
        format_date($dblog->timestamp, 'large'),
      ),
      array(
        array('data' => t('User'), 'header' => TRUE),
        theme('username', $dblog),
      ),
      array(
        array('data' => t('Location'), 'header' => TRUE),
        l($dblog->location, $dblog->location),
      ),
      array(
        array('data' => t('Referrer'), 'header' => TRUE),
        l($dblog->referer, $dblog->referer),
      ),
      array(
        array('data' => t('Message'), 'header' => TRUE),
        _dblog_format_message($dblog),
      ),
      array(
        array('data' => t('Severity'), 'header' => TRUE),
        $severity[$dblog->severity],
      ),
      array(
        array('data' => t('Hostname'), 'header' => TRUE),
        check_plain($dblog->hostname),
      ),
      array(
        array('data' => t('Operations'), 'header' => TRUE),
        $dblog->link,
      ),
    );
    $attributes = array('class' => 'dblog-event');
    $output = theme('table', array(), $rows, $attributes);
  }
  return $output;
}

/**
 * Build query for dblog administration filters based on session.
 */
function dblog_build_filter_query() {
  if (empty($_SESSION['dblog_overview_filter'])) {
    return;
  }

  $filters = dblog_filters();

  // Build query
  $where = $args = array();
  foreach ($_SESSION['dblog_overview_filter'] as $key => $filter) {
    $filter_where = array();
    foreach ($filter as $value) {
      $filter_where[] = $filters[$key]['where'];
      $args[] = $value;
    }
    if (!empty($filter_where)) {
      $where[] = '(' . implode(' OR ', $filter_where) . ')';
    }
  }
  $where = !empty($where) ? implode(' AND ', $where) : '';

  return array(
    'where' => $where,
    'args' => $args,
  );
}


/**
 * List dblog administration filters that can be applied.
 */
function dblog_filters() {
  $filters = array();

  foreach (_dblog_get_message_types() as $type) {
    $types[$type] = $type;
  }

  if (!empty($types)) {
    $filters['type'] = array(
      'title' => t('Type'),
      'where' => "w.type = ':s'",
      'options' => $types,
    );
  }

  $filters['severity'] = array(
    'title' => t('Severity'),
    'where' => 'w.severity = :d',
    'options' => watchdog_severity_levels(),
  );

  return $filters;
}

/**
 * Formats a log message for display.
 *
 * @param $dblog
 *   An object with at least the message and variables properties
 */
function _dblog_format_message($dblog) {
  // Legacy messages and user specified text
  if ($dblog->variables === 'N;') {
    return $dblog->message;
  }
  // Message to translate with injected variables
  else {
    return t($dblog->message, unserialize($dblog->variables));
  }
}


/**
 * Return form for dblog administration filters.
 *
 * @ingroup forms
 * @see dblog_filter_form_submit()
 * @see dblog_filter_form_validate()
 */
function dblog_filter_form() {
  if (!isset($_SESSION['dblog_overview_filter'])) {
    drupal_set_session('dblog_overview_filter', array());
  }
  $session = &$_SESSION['dblog_overview_filter'];
  $filters = dblog_filters();

  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filter log messages'),
    '#theme' => 'dblog_filters',
    '#collapsible' => TRUE,
    '#collapsed' => empty($session),
  );
  foreach ($filters as $key => $filter) {
    $form['filters']['status'][$key] = array(
      '#title' => $filter['title'],
      '#type' => 'select',
      '#multiple' => TRUE,
      '#size' => 8,
      '#options' => $filter['options'],
    );
    if (!empty($session[$key])) {
      $form['filters']['status'][$key]['#default_value'] = $session[$key];
    }
  }

  $form['filters']['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
  );
  if (!empty($session)) {
    $form['filters']['buttons']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset')
    );
  }

  return $form;
}

/**
 * Validate result from dblog administration filter form.
 */
function dblog_filter_form_validate($form, &$form_state) {
  if ($form_state['values']['op'] == t('Filter') && empty($form_state['values']['type']) && empty($form_state['values']['severity'])) {
    form_set_error('type', t('You must select something to filter by.'));
  }
}

/**
 * Process result from dblog administration filter form.
 */
function dblog_filter_form_submit($form, &$form_state) {
  $op = $form_state['values']['op'];
  $filters = dblog_filters();
  switch ($op) {
    case t('Filter'):
      foreach ($filters as $name => $filter) {
        if (isset($form_state['values'][$name])) {
          if (!isset($_SESSION['dblog_overview_filter'])) {
            drupal_set_session('dblog_overview_filter', array());
          }
          $_SESSION['dblog_overview_filter'][$name] = $form_state['values'][$name];
        }
      }
      break;
    case t('Reset'):
      drupal_set_session('dblog_overview_filter', array());
      break;
  }
  return 'admin/reports/dblog';
}

/**
 * Return form for dblog clear button.
 *
 * @ingroup forms
 * @see dblog_clear_log_submit()
 */
function dblog_clear_log_form() {
  $form['dblog_clear'] = array(
    '#type' => 'fieldset',
    '#title' => t('Clear log messages'),
    '#description' => t('This will permanently remove the log messages from the database.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['dblog_clear']['clear'] = array(
    '#type' => 'submit',
    '#value' => t('Clear log messages'),
    '#submit' => array('dblog_clear_log_submit'),
  );

  return $form;
}

/**
 * Submit callback: clear database with log messages.
 */
function dblog_clear_log_submit(&$form_state, $form) {
  db_delete('watchdog')->execute();
  drupal_set_message(t('Database log cleared.'));
}
