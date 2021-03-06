<?php
// $Id: tracker.pages.inc,v 1.18 2009-04-26 19:44:40 dries Exp $

/**
 * @file
 * User page callbacks for the tracker module.
 */


/**
 * Menu callback. Prints a listing of active nodes on the site.
 */
function tracker_page($account = NULL, $set_title = FALSE) {
  // Add CSS
  drupal_add_css(drupal_get_path('module', 'tracker') . '/tracker.css', array('preprocess' => FALSE));

  $header = array(
    t('Type'),
    t('Post'),
    t('Author'),
    t('Replies'),
    t('Last updated')
  );

  // TODO: These queries are very expensive, see http://drupal.org/node/105639
  $query = db_select('node', 'n', array('target' => 'slave'))
    ->fields('n', array('nid', 'title', 'type', 'changed', 'uid'));
  $query->join('users', 'u', 'n.uid = u.uid');
  $query->addField('u', 'name');
  $query->join('node_comment_statistics', 'l', 'n.nid = l.nid');
  $query->addExpression('GREATEST(n.changed, l.last_comment_timestamp)', 'last_updated');
  $query->addField('l', 'comment_count');
  $query->orderBy('last_updated', 'DESC');

  if ($account) {
    if ($set_title) {
      // When viewed from user/%user/track, display the name of the user
      // as page title -- the tab title remains Track so this needs to be done
      // here and not in the menu definition.
      drupal_set_title($account->name);
    }
    $query->leftJoin('comment', 'c', 'n.nid = c.nid AND (c.status = :status OR c.status IS NULL)', array(':status' => COMMENT_PUBLISHED));
    $query->condition(db_or()
      ->condition('n.uid', $account->uid)
      ->condition('c.uid', $account->uid)
    );
  }

  $query->condition('n.status', 1);
  $query->distinct();
  $query->extend('PagerDefault')->extend('TableSort')
    ->limit(25, 0)
    ->setHeader($header);

  $result = $query->execute();

  $rows = array();
  foreach ($result as $node) {
    // Determine the number of comments:
    $comments = 0;
    if ($node->comment_count) {
      $comments = $node->comment_count;

      if ($new = comment_num_new($node->nid)) {
        $comments .= '<br />';
        $comments .= l(format_plural($new, '1 new', '@count new'), "node/$node->nid", array('query' => comment_new_page_count($node->comment_count, $new, $node), 'fragment' => 'new'));
      }
    }

    $rows[] = array(
      check_plain(node_get_types('name', $node->type)),
      l($node->title, "node/$node->nid") . ' ' . theme('mark', node_mark($node->nid, $node->changed)),
      theme('username', $node),
      array('class' => 'replies', 'data' => $comments),
      t('!time ago', array('!time' => format_interval(REQUEST_TIME - $node->last_updated)))
    );
  }

  if (!$rows) {
    $rows[] = array(array('data' => t('No posts available.'), 'colspan' => '5'));
  }

  $output = '<div id="tracker">';
  $output .= theme('table', $header, $rows);
  $output .= theme('pager', NULL);
  $output .= '</div>';

  return $output;
}
